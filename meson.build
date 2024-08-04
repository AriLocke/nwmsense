project('compositions', 'c', version : '1.0.0', default_options : ['warning_level=3', 'c_std=c17'])

#Use wayland-scanner to generate C headers for xdg-shell
wayland_protocols = run_command('pkg-config', '--variable=pkgdatadir', 'wayland-protocols').stdout().strip()
wayland_scanner = run_command('pkg-config', '--variable=wayland_scanner', 'wayland-scanner').stdout().strip()

run_command(wayland_scanner, 'server-header', wayland_protocols + '/stable/xdg-shell/xdg-shell.xml', 'xdg-shell-protocol.h')
run_command(wayland_scanner, 'private-code', wayland_protocols + '/stable/xdg-shell/xdg-shell.xml', 'compositions') # unnecessary?

### Import wlroots through pkgconfig
#pkg = import('pkgconfig')
wlroots_dep = dependency('wlroots-0.18')
wayland_server_dep = dependency('wayland-server')
xkbcommon_dep = dependency('xkbcommon')

# Add project arguments
add_project_arguments([ '-DWLR_USE_UNSTABLE' ], language: 'c')

# Source files
src = ['src/main.c',
       'src/comp_server.c',
       'src/comp_output.c',
       'src/comp_xdg_shell.c']

## Setup Compile Target
compositions = executable('compositions',
                          src,#, protocols_server_header['xdg-shell']],
                          install : true,
                          dependencies : [wlroots_dep,
                                          wayland_server_dep,
                                          xkbcommon_dep], )