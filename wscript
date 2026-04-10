# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    # Build the gradient routing protocol library
    obj = bld.create_ns3_program(
        'routing-sim',
        ['core', 'network', 'internet', 'applications', 'mobility', 'lr-wpan', 'dsdv', 'aodv']
    )
    
    obj.source = [
        'gradient.cc',
        'routing_sim.cc',
    ]
    
    obj.includes = ['.']
