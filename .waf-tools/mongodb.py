#! /usr/bin/env python
# encoding: utf-8

from waflib.Configure import conf


@conf
def check_mongodb(self, *k, **kw):
    self.check_cfg(package='libmongocxx',
                   args=['--cflags', '--libs'],
                   uselib_store='MONGODB',
                   mandatory=True)
