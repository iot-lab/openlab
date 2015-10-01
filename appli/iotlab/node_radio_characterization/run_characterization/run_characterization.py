#! /usr/bin/env python
# -*- coding:utf-8 -*-

""" Run radio characterizations on nodes """

import os
import sys
import time

from iotlabaggregator.serial import SerialAggregator
import iotlabaggregator.common

FIRMWARE_PATH = "node_radio_characterization.elf"

class RadioCharac(object):
    """ Radio Characterization """
    def __init__(self, nodes_list):
        self.state = {}
        self.nodes = SerialAggregator(nodes_list)
        self.current_sender_node = None

    def start(self):
        """ Start nodes connection """
        self.nodes.start()

    def stop(self):
        """ Stop nodes connection """
        self.nodes.stop()

    def _answers_handler(self, node_id, msg):
        """ Handle answers """
        try:
            line = msg.split(' ')

            if line[0] == 'config_radio':
                if line[1] == 'ACK':
                    self.state['config']['success'].append(node_id)
                else:
                    self.state['config']['failure'].append(node_id)
            elif line[0] == 'send_packets':
                if line[1] == 'ACK':
                    # send_packets ACK 0
                    self.state['send']['success'].append(node_id)
                else:
                    # send_packets NACK 42
                    self.state['send']['failure'].append('%s %s' %
                                                         (node_id, line[2]))

            elif line[0] == 'radio_rx':
                # "radio_rx m3-128 -17dBm 5 -61 255 sender power num rssi lqi"
                sender = line[1]

                # add list for this node
                results = self.state['radio'][sender].get(
                    node_id, {'success': [], 'errors': []})
                self.state['radio'][sender][node_id] = results

                # add rx informations
                #results.append("%s" % ' '.join(line[2:6]))
                results['success'].append(tuple(line[2:6]))
            elif line[0] == 'radio_rx_error':
                sender = self.current_sender_node.node_id

                # add list for this node
                results = self.state['radio'][sender].get(
                    node_id, {'success': [], 'errors': []})
                self.state['radio'][sender][node_id] = results

                results['errors'].append("%s" % line[1])

                # print >> sys.stderr, "Radio_rx_error %s %s sender %s" % (
                #         node_id, line[1], self.current_sender_node.node_id)

            else:
                print >> sys.stderr, "UNKOWN_MSG: %s %r" % (node_id, msg)

        except IndexError:
            print >> sys.stderr, "UNKOWN_MSG: %s %r" % (node_id, msg)


    def run_characterization(self, channel, power, num_pkts, delay):
        """ Run the radio characterizations on nodes"""

        self.start()

        self.state['options'] = {'power': power, 'channel': channel,
                                 'num_pkts': num_pkts, 'delay': delay}

        self.state['config'] = {'success': [], 'failure': []}
        self.state['send'] = {'success': [], 'failure': []}

        #nodes = self.nodes.values()
        #self.nodes_cli('--update', firmware=FIRMWARE_PATH)
        #time.sleep(2)  # wait started

        # init all nodes handlers and radio config
        self.state['radio'] = {}

        for node in self.nodes.values():
            self.state['radio'][node.node_id] = {}
            node.line_handler.append(self._answers_handler)

        cmd = "config_radio -c {channel}\n"
        self.nodes.broadcast(cmd.format(channel=channel))

        time.sleep(10)  # wait

        cmd = "send_packets -i {node_id} -p {power} -n {num_pkts} -d {delay}\n"
        for node in self.nodes.values():
            self.current_sender_node = node
            print >> sys.stderr, "sending node %s" % node.node_id
            node.send(cmd.format(node_id=node.node_id, power=power,
                                 num_pkts=num_pkts, delay=delay))
            time.sleep(2)
            self.current_sender_node = None


        self.stop()
        return self.state

def simple_results_summary(result, human_readable=False):
    """ Parse outputs to be readable by a human """
    num_pkt = result['options']['num_pkts']
    for sender_node in result['radio'].values():
        for rx_node in sender_node:

            node_result = {}
            raw_result = sender_node[rx_node]['success']
            raw_errors = sender_node[rx_node]['errors']

            if len(raw_result):
                # Average RSSI
                average_rssi = sum([int(res[2]) for res in raw_result]) \
                    / float(len(raw_result))
                node_result['average_rssi'] = average_rssi

                # Packet loss
                rx_pkt = 100 * len(raw_result) / float(num_pkt)
                node_result['pkt_reception'] = "%.1f %%" % rx_pkt

            if len(raw_errors):
                # errors
                node_result['errors'] = len(raw_errors)

            sender_node[rx_node] = node_result

            if human_readable:
                # increase readability when using command line
                sender_node[rx_node] = "%s" % sender_node[rx_node]

    return result


def main(argv):
    """ Run a characterization script on all nodes from an experiment """

    json_dict = json.loads(sys.stdin.read())
    nodes_list = iotlabaggregator.common.extract_nodes(json_dict, os.uname()[1])

    rad_charac = RadioCharac(nodes_list)

    num_pkt = 32
    result = rad_charac.run_characterization(channel=16, power="-17dBm",
                                             num_pkts=32, delay=10)

    if '--summary' in argv:
        result = simple_results_summary(result, human_readable=('-h' in argv))

    import json
    result_str = json.dumps(result, sort_keys=True, indent=4)
    print result_str


if __name__ == "__main__":
    main(sys.argv)
