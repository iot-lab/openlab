#! /bin/bash

cd $(dirname $0)

ROOT_IOTLAB="$(readlink -e $(git rev-parse --show-toplevel)/../../)"

PYTHONPATH="."
export PYTHONPATH

experiment-cli get -r | python run_characterization.py $@
