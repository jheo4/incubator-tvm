from __future__ import absolute_import, print_function

import argparse, json, os, requests, sys, time
from io import BytesIO
from os.path import join, isfile
from PIL import Image

from mxnet.gluon.model_zoo import vision
import numpy as np
from matplotlib import pyplot as plt

import tvm
from tvm import te
from tvm import rpc, autotvm, relay
from tvm.contrib import graph_runtime, util, download
from tvm.contrib.debugger import debug_runtime
from tvm.relay import transform

import vta
from vta.testing import simulator
from vta.top import graph_pack

# Make sure that TVM was compiled with RPC=1
assert tvm.runtime.enabled("rpc")

env = vta.get_env()
device = "vta"
target = env.target
pack_dict = {
    "resnet18_v1": ["nn.max_pool2d", "nn.global_avg_pool2d"],
    "resnet34_v1": ["nn.max_pool2d", "nn.global_avg_pool2d"],
    "resnet18_v2": ["nn.max_pool2d", "nn.global_avg_pool2d"],
    "resnet34_v2": ["nn.max_pool2d", "nn.global_avg_pool2d"],
    "resnet50_v2": ["nn.max_pool2d", "nn.global_avg_pool2d"],
    "resnet101_v2": ["nn.max_pool2d", "nn.global_avg_pool2d"],
}

model = "resnet18_v1"
assert model in pack_dict

remote = rpc.LocalSession()
ctx = remote.ext_dev(0)

with autotvm.tophub.context(target):
    dtype_dict = {"data": 'float32'}
    shape_dict = {"data": (env.BATCH, 3, 224, 224)}

    # Get off the shelf gluon model, and convert to relay
    gluon_model = vision.get_model(model, pretrained=True)

    # Measure build start time
    build_start = time.time()

    # Start front end compilation
    mod, params = relay.frontend.from_mxnet(gluon_model, shape_dict)

    # Update shape and type dictionary
    shape_dict.update({k: v.shape for k, v in params.items()})
    dtype_dict.update({k: str(v.dtype) for k, v in params.items()})

    if target.device_name == "vta":
        with relay.build_config(opt_level=3):
            with relay.quantize.qconfig(global_scale=8.0,
                                        skip_conv_layers=[0]):
                mod = relay.quantize.quantize(mod, params=params)
            # Perform graph packing and constant folding for VTA target
            assert env.BLOCK_IN == env.BLOCK_OUT
            relay_prog = graph_pack(
                mod["main"],
                env.BATCH,
                env.BLOCK_OUT,
                env.WGT_WIDTH,
                start_name=pack_dict[model][0],
                stop_name=pack_dict[model][1])
    else:
        relay_prog = mod["main"]

    # Compile Relay program with AlterOpLayout disabled
    with relay.build_config(opt_level=3, disabled_pass={"AlterOpLayout"}):
        if target.device_name != "vta":
            graph, lib, params = relay.build(
                relay_prog, target=target,
                params=params, target_host=env.target_host)
        else:
            with vta.build_config():
                graph, lib, params = relay.build(
                    relay_prog, target=target,
                    params=params, target_host=env.target_host)

    # Measure Relay build time
    build_time = time.time() - build_start
    print(model + " inference graph built in {0:.2f}s!".format(build_time))

    # Send the inference library over to the remote RPC server
    temp = util.tempdir()
    lib.save(temp.relpath("graphlib.o"))
    remote.upload(temp.relpath("graphlib.o"))
    lib = remote.load_module("graphlib.o")

    # Graph runtime
    m = graph_runtime.create(graph, lib, ctx)

# Download ImageNet categories
categ_url = "https://github.com/uwsaml/web-data/raw/master/vta/models/"
categ_fn = "synset.txt"
download.download(join(categ_url, categ_fn), categ_fn)
synset = eval(open(categ_fn).read())

# Download test image
image_url = 'https://homes.cs.washington.edu/~moreau/media/vta/cat.jpg'
image_fn = 'cat.png'
download.download(image_url, image_fn)

# Prepare test image for inference
image = Image.open(image_fn).resize((224, 224))
image = np.array(image) - np.array([123., 117., 104.])
image /= np.array([58.395, 57.12, 57.375])
image = image.transpose((2, 0, 1))
image = image[np.newaxis, :]
image = np.repeat(image, env.BATCH, axis=0)

# Set the network parameters and inputs
m.set_input(**params)
m.set_input('data', image)

# Perform inference and gather execution statistics
# More on: :py:method:`tvm.runtime.Module.time_evaluator`
num = 1 # number of times we run module for a single measurement
rep = 1 # number of measurements (we derive std dev from this)
# 1 inference
timer = m.module.time_evaluator("run", ctx, number=num, repeat=rep)

if env.TARGET in ["sim", "tsim"]:
    tcost = timer()
    std = np.std(tcost.results) * 1000
    mean = tcost.mean * 1000
    print("\nPerformed inference in %.2fms (std = %.2f) for %d samples" % (mean, std, env.BATCH))
    print("Average per sample inference time: %.2fms" % (mean/env.BATCH))

# Get classification results
tvm_output = m.get_output(0, tvm.nd.empty((env.BATCH, 1000), "float32", remote.cpu(0)))
for b in range(env.BATCH):
    top_categories = np.argsort(tvm_output.asnumpy()[b])
    # Report top-5 classification results
    print("\n{} prediction for sample {}".format(model, b))
    print("\t#1:", synset[top_categories[-1]])
    print("\t#2:", synset[top_categories[-2]])
    print("\t#3:", synset[top_categories[-3]])
    print("\t#4:", synset[top_categories[-4]])
    print("\t#5:", synset[top_categories[-5]])
    # This just checks that one of the 5 top categories
    # is one variety of cat; this is by no means an accurate
    # assessment of how quantization affects classification
    # accuracy but is meant to catch changes to the
    # quantization pass that would accuracy in the CI.
    cat_detected = False
    for k in top_categories[-5:]:
        if "cat" in synset[k]:
            cat_detected = True
