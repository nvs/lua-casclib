rockspec_format = '3.0'

package = 'lua-casclib'
version = 'scm-0'

description = {
	summary = 'A Lua binding to CascLib',
	homepage = 'https://github.com/nvs/lua-casclib',
	license = 'MIT'
}

source = {
	url = 'git+https://github.com/nvs/lua-casclib.git'
}

dependencies = {
	'lua >= 5.1, < 5.4',
}

build = {
   type = "builtin",
   modules = {
		['casclib'] = {
			sources = {
				'src/common.c',
				'src/init.c',
				'src/file.c',
				'src/finder.c',
				'src/registry.c',
				'src/storage.c',
				'lib/compat-5.3/c-api/compat-5.3.c'
			},
			libraries = {
				'casc'
			},
			incdirs = {
				'lib/compat-5.3/c-api',
			}
		}
   }
}
