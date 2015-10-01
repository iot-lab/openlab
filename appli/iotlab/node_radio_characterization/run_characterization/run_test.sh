#! /bin/bash

cd $(dirname $0)

ROOT_IOTLAB="$(readlink -e $(git rev-parse --show-toplevel)/../../)"

PYTHONPATH="${ROOT_IOTLAB}/tools_and_scripts/:${ROOT_IOTLAB}/cli-tools/:${ROOT_IOTLAB}/aggregation-tools/:."
export PYTHONPATH

experiment-cli get -r | python run_characterization.py $@
