
from iotlabaggregator import serial

def opts_parser():
    """ Argument parser object """
    import argparse
    parser = argparse.ArgumentParser()
    common_parser.add_auth_arguments(parser)

    nodes_group = parser.add_argument_group(
        description="By default, select currently running experiment nodes",
        title="Nodes selection")

    nodes_group.add_argument('-i', '--id', dest='experiment_id', type=int,
                             help='experiment id submission')

    nodes_group.add_argument(
        '-l', '--list', type=common_parser.nodes_list_from_str,
        dest='nodes_list', help='nodes list, may be given multiple times')

    nodes_group.add_argument('-a', '--algo', default='syncronous',
                             choices=ALGOS.keys(), help='Algorithm to run')

    nodes_group.add_argument(
        '-n', '--num-loop', type=int, required=True,
        dest='num_loop', help='number_of_loops_to_run')

    nodes_group.add_argument(
        '-o', '--out-file', required=True,
        dest='outfile', help='Files where to output traces')

    return parser

def main():

	parser = opts_parser()
    opts = parser.parse_args()

    opts.with_a8 = False # needed, as we are not using a8 nodes

	nodes_list = SerialAggregator.select_nodes(opts)
	pass

if __name__ == '__main__':
	main()