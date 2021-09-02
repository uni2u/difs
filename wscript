# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib import Utils, Context
import os, subprocess

VERSION = '0.1'
GIT_TAG_PREFIX = 'ndn-difs-'

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags', 'coverage', 'sanitizers', 'boost', 'sqlite3', 'mongodb'],
             tooldir=['.waf-tools'])

    optgrp = opt.add_option_group('Repo-ng Options')
    optgrp.add_option('--with-examples', action='store_true', default=False,
                      help='Build examples')
    optgrp.add_option('--with-tests', action='store_true', default=False,
                      help='Build unit tests')
    optgrp.add_option('--without-tools', action='store_false', default=True, dest='with_tools',
                      help='Do not build tools')
    opt.add_option('--enable-static', action='store_true', default=False,
                   dest='enable_static', help='Build static library (disabled by default)')
    opt.add_option('--disable-static', action='store_false', default=False,
                   dest='enable_static', help='Do not build static library (disabled by default)')

    opt.add_option('--enable-shared', action='store_true', default=True,
                   dest='enable_shared', help='Build shared library (enabled by default)')
    opt.add_option('--disable-shared', action='store_false', default=True,
                   dest='enable_shared', help='Do not build shared library (enabled by default)')

    opt.add_option('--enable-storage', action='store_true', default=True,
                   dest='enable_storage', help='Build storage application (enabled by default)')
    opt.add_option('--disable-storage', action='store_false', default=True,
                   dest='enable_storage', help='Do not build storage application (enabled by default)')

def configure(conf):
    conf.start_msg('Building static library')
    if conf.options.enable_static:
        conf.end_msg('yes')
    else:
        conf.end_msg('no', color='YELLOW')
    conf.env.enable_static = conf.options.enable_static

    conf.start_msg('Building shared library')
    if conf.options.enable_shared:
        conf.end_msg('yes')
    else:
        conf.end_msg('no', color='YELLOW')
    conf.env.enable_shared = conf.options.enable_shared

    conf.start_msg('Building Storage')
    if conf.options.enable_storage:
        conf.end_msg('yes')
    else:
        conf.end_msg('no', color='YELLOW')
    conf.env.enable_storage = conf.options.enable_storage

    if not conf.options.enable_shared and not conf.options.enable_static:
        conf.fatal('Either static library or shared library must be enabled')

    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'boost', 'sqlite3', 'mongodb'])

    conf.env['WITH_EXAMPLES'] = conf.options.with_examples
    conf.env['WITH_TESTS'] = conf.options.with_tests
    conf.env['WITH_TOOLS'] = conf.options.with_tools

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'], uselib_store='NDN_CXX',
                   pkg_config_path=os.environ.get('PKG_CONFIG_PATH', '%s/pkgconfig' % conf.env.LIBDIR))

    if conf.options.enable_storage:
        conf.check_sqlite3()
        conf.check_mongodb()

    USED_BOOST_LIBS = ['system', 'program_options', 'iostreams', 'filesystem', 'thread', 'log']
    if conf.env['WITH_TESTS']:
        USED_BOOST_LIBS += ['unit_test_framework']
    conf.check_boost(lib=USED_BOOST_LIBS, mt=True)

    conf.check_compiler_flags()

    # Loading "late" to prevent tests from being compiled with profiling flags
    conf.load('coverage')
    conf.load('sanitizers')

    conf.define('DEFAULT_CONFIG_FILE', '%s/ndn/repo-ng.conf' % conf.env['SYSCONFDIR'])
    conf.define_cond('DISABLE_SQLITE3_FS_LOCKING', not conf.options.with_sqlite_locking)
    conf.define_cond('HAVE_TESTS', conf.env['WITH_TESTS'])

    conf.write_config_header('src/config.hpp')

    conf.recurse('tools')

def build(bld):
    version(bld)

    if bld.env.enable_storage:
        print('Build NDN DIFS Objects')
        bld.objects(target='repo-objects',
                source=bld.path.ant_glob('src/**/*.cpp',
                                         excl=['src/main.cpp']),
                use='NDN_CXX BOOST SQLITE3 MONGODB',
                includes='src',
                export_includes='src')

    if bld.env.enable_storage:
        print('Build NDN DIFS')
        bld.program(name='ndn-difs`',
                target='bin/ndn-difs',
                source='src/main.cpp',
                use='repo-objects')


    ########################################## Build tools library ###########################################
    print('Build NDN DIFS Library Object')
    libndn_difs = dict(
        target='ndn-difs',
        source=
            bld.path.ant_glob('lib/*.cpp') +
            bld.path.find_node('src').ant_glob('repo-command*.cpp') +
            bld.path.find_node('src').ant_glob('manifest/*.cpp') +
            bld.path.find_node('src').ant_glob('util.cpp'),
        use='version BOOST OPENSSL SQLITE3 ATOMIC RT PTHREAD NDN_CXX',
        includes='src',
        export_includes='lib',
        install_path='${LIBDIR}')

    if bld.env.enable_shared:
        print('Build Shared NDN DIFS Library')
        bld.shlib(name='ndn-difs',
                  vnum=VERSION_BASE,
                  cnum=VERSION_BASE,
                  **libndn_difs)

    if bld.env.enable_static:
        print('Build Static NDN DIFS Library')
        bld.stlib(name='ndn-difs-static' if bld.env.enable_shared else 'ndn-difs',
                  **libndn_difs)

    # Prepare flags that should go to pkgconfig file
    pkgconfig_libs = []
    pkgconfig_ldflags = []
    pkgconfig_linkflags = []
    pkgconfig_includes = []
    pkgconfig_difsflags = []
    pkgconfig_defines = []
    for lib in Utils.to_list(libndn_difs['use']):
        if bld.env['LIB_%s' % lib]:
            pkgconfig_libs += Utils.to_list(bld.env['LIB_%s' % lib])
        if bld.env['LIBPATH_%s' % lib]:
            pkgconfig_ldflags += Utils.to_list(bld.env['LIBPATH_%s' % lib])
        if bld.env['INCLUDES_%s' % lib]:
            pkgconfig_includes += Utils.to_list(bld.env['INCLUDES_%s' % lib])
        if bld.env['LINKFLAGS_%s' % lib]:
            pkgconfig_linkflags += Utils.to_list(bld.env['LINKFLAGS_%s' % lib])
        if bld.env['CXXFLAGS_%s' % lib]:
            pkgconfig_difsflags += Utils.to_list(bld.env['CXXFLAGS_%s' % lib])
        if bld.env['DEFINES_%s' % lib]:
            pkgconfig_defines += Utils.to_list(bld.env['DEFINES_%s' % lib])

    EXTRA_FRAMEWORKS = ''
    if bld.env.HAVE_OSX_FRAMEWORKS:
        EXTRA_FRAMEWORKS = '-framework CoreFoundation -framework Security -framework SystemConfiguration -framework Foundation -framework CoreWLAN'

    def uniq(alist):
        seen = set()
        return [x for x in alist if x not in seen and not seen.add(x)]

    pkconfig = bld(features='subst',
         source='libndn-difs.pc.in',
         target='libndn-difs.pc',
         install_path='${LIBDIR}/pkgconfig',
         VERSION=VERSION_BASE,

         # This probably not the right thing to do, but to simplify life of apps
         # that use the library
         EXTRA_LIBS=' '.join([('-l%s' % i) for i in uniq(pkgconfig_libs)]),
         EXTRA_LDFLAGS=' '.join([('-L%s' % i) for i in uniq(pkgconfig_ldflags)]),
         EXTRA_LINKFLAGS=' '.join(uniq(pkgconfig_linkflags)),
         EXTRA_INCLUDES=' '.join([('-I%s' % i) for i in uniq(pkgconfig_includes)]),
         EXTRA_CXXFLAGS=' '.join(uniq(pkgconfig_difsflags) + [('-D%s' % i) for i in uniq(pkgconfig_defines)]),
         EXTRA_FRAMEWORKS=EXTRA_FRAMEWORKS)
    #######################################################################################################

    if bld.env.WITH_TESTS:
        bld.recurse('tests')

    if bld.env.WITH_TOOLS:
        bld.recurse('tools')

    if bld.env.WITH_EXAMPLES:
        bld.recurse('examples')

    bld.install_files('${SYSCONFDIR}/ndn', 'repo-ng.conf.sample')

    if Utils.unversioned_sys_platform() == 'linux':
        bld(features='subst',
            name='repo-ng.service',
            source='systemd/repo-ng.service.in',
            target='systemd/repo-ng.service')



def version(ctx):
    # don't execute more than once
    if getattr(Context.g_module, 'VERSION_BASE', None):
        return

    Context.g_module.VERSION_BASE = Context.g_module.VERSION
    Context.g_module.VERSION_SPLIT = VERSION_BASE.split('.')

    # first, try to get a version string from git
    gotVersionFromGit = False
    try:
        cmd = ['git', 'describe', '--always', '--match', '%s*' % GIT_TAG_PREFIX]
        out = subprocess.check_output(cmd, universal_newlines=True).strip()
        if out:
            gotVersionFromGit = True
            if out.startswith(GIT_TAG_PREFIX):
                Context.g_module.VERSION = out.lstrip(GIT_TAG_PREFIX)
            else:
                # no tags matched
                Context.g_module.VERSION = '%s-commit-%s' % (VERSION_BASE, out)
    except (OSError, subprocess.CalledProcessError):
        pass

    versionFile = ctx.path.find_node('VERSION.info')
    if not gotVersionFromGit and versionFile is not None:
        try:
            Context.g_module.VERSION = versionFile.read()
            return
        except EnvironmentError:
            pass

    # version was obtained from git, update VERSION file if necessary
    if versionFile is not None:
        try:
            if versionFile.read() == Context.g_module.VERSION:
                # already up-to-date
                return
        except EnvironmentError as e:
            Logs.warn('%s exists but is not readable (%s)' % (versionFile, e.strerror))
    else:
        versionFile = ctx.path.make_node('VERSION.info')

    try:
        versionFile.write(Context.g_module.VERSION)
    except EnvironmentError as e:
        Logs.warn('%s is not writable (%s)' % (versionFile, e.strerror))
