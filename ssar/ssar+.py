#!/usr/bin/env python
# encoding: utf-8

import os
import sys
import argparse

MAX_SUB_HELP_POSITION = 45
MAX_HELP_WIDTH = 150

class SeqOptions(object):
    pass

class ItFormatter(argparse.RawTextHelpFormatter):
    def format_help(self):
        help = self._root_section.format_help()
        if help:
            help = self._long_break_matcher.sub('\n\n', help)
            help = help.strip('\n') + '\n'
        return help

def sysfunc(args):
    print('ssar+ is the future of tsar2')
    #print('sysfunc === ', args)
def workers(args):
    print('ssar+ is the future of tsar2.', args)
    #print('workers === ', args)
def procs(args):
    print('ssar+ is the future of tsar2.', args)
    #print('procs === ', args)

seq_option = SeqOptions()

def main():
    subcommand_list = ['procs', 'workers']
    subcommand_info = "Subcommands:" + "\n"
    for it_subcommand in subcommand_list:
        subcommand_info = subcommand_info + "  ssar " + it_subcommand + " -h\n"

    root_footer_info = '''
Examples:
  ssar --cpu
  ssar --mem
'''
    procs_footer_info = '''
Examples:
  ssar procs --cpu1
  ssar procs --mem1
'''

    # ItFormatterClass = lambda prog: ItFormatter(prog, max_help_position = MAX_SUB_HELP_POSITION, width = MAX_HELP_WIDTH)
    ItFormatterClass = lambda prog: ItFormatter(prog)
    root_parser = argparse.ArgumentParser(add_help = False, formatter_class = ItFormatterClass, usage = "%(prog)s [OPTIONS] [SUBCOMMAND]", epilog = subcommand_info + root_footer_info)
    root_parser.add_argument("-h", "--help", action = "store_true", dest = "root_help", default = False, help = "Display help information")
    root_parser.add_argument("--cpu", action = "store_true", dest = "seq_option.cpu", help = "Display cpu information")
    root_parser.add_argument("--mem", action = "store_true", dest = "seq_option.mem", help = "Display mem information")

    common_parser = argparse.ArgumentParser()
    subparsers = common_parser.add_subparsers(dest='cmd')

    procs_parser = subparsers.add_parser("procs", help = "procs subcommand", formatter_class = ItFormatterClass, epilog = procs_footer_info)
    procs_parser.add_argument("-o", "--output", help = "output info")
    procs_parser.add_argument("--cpu", action = "store_true", dest = "seq_option.cpu", help = "Display procs cpu information")
    procs_parser.add_argument("--mem", action = "store_true", dest = "seq_option.mem", help = "Display procs mem information")
    procs_parser.set_defaults(callback = procs)

    workers_parser = subparsers.add_parser("workers", help = "workers Command")
    workers_parser.set_defaults(callback = workers)

    root_args, root_extras = root_parser.parse_known_args()
    if len(root_extras) > 0:
        if root_extras[0] in subcommand_list:
            common_args, common_extras = common_parser.parse_known_args()
            if len(common_extras) > 0:
                subparsers._name_parser_map[root_extras[0]].print_help()        # procs_parser.print_help()
                exit(201)
            else:
                common_args.callback(common_args)
        else:
            root_parser.print_help()
            exit(202)
    else:
        if root_args.root_help:
            root_parser.print_help()
            exit(203)
        else:
            sysfunc(root_args)

if __name__ == "__main__":
	main()
