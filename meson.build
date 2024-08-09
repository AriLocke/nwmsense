project('nwm', 'c', version : '1.0.0', default_options : ['warning_level=3', 'c_std=c17'])

#Use wayland-scanner to generate C headers for xdg-shell
wayland_protocols = run_command('pkg-config', '--variable=pkgdatadir', 'wayland-protocols').stdout().strip()
wayland_scanner = run_command('pkg-config', '--variable=wayland_scanner', 'wayland-scanner').stdout().strip()

run_command(wayland_scanner, 'server-header', wayland_protocols + '/stable/xdg-shell/xdg-shell.xml', 'libs/xdg-shell-protocol.h')
run_command(wayland_scanner, 'private-code', wayland_protocols + '/stable/xdg-shell/xdg-shell.xml', 'libs/xdg-shell-protocol.c') # unnecessary?

### Import wlroots through pkgconfig
#pkg = import('pkgconfig')
wlroots_dep = dependency('wlroots-0.18')
wayland_server_dep = dependency('wayland-server')
xkbcommon_dep = dependency('xkbcommon')

# Add project arguments
add_project_arguments([ '-DWLR_USE_UNSTABLE' ], language: 'c')

# Source files
src = [ 'src/main.c',
        'src/nwm_server.c',
        'src/output.c',
        'src/xdg_shell.c',
        'src/input/cursor.c',
        'src/input/seat.c',
        'src/input/input.c',
        'src/input/keyboard.c']

incdir = include_directories('libs')

## Setup Compile Target
compositions = executable('nwm',
                          sources : src,#, protocols_server_header['xdg-shell']],
                          include_directories : incdir,
                          install : true,
                          dependencies : [wlroots_dep,
                                          wayland_server_dep,
                                          xkbcommon_dep], )