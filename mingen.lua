if mg.need_generate() then
	mg.configurations({"debug_coverage", "debug", "release"})
end

local mincore = mg.project({
	name = "mincore",
	type = mg.project_type.sources,
	sources = {"src/**.cc"},
	compile_options = {"-g", "-Wall", "-Wextra", "-nostdinc++", "--std=c++20", '-D"_MT"', '-D"_DLL"'},
	debug_coverage = {
		compile_options = {"-O0", "-fprofile-instr-generate", "-fcoverage-mapping"}
	},
	debug = {
		compile_options = {"-O0", "-Werror"}
	},
	release = {
		compile_options = {"-O2", "-Werror"}
	},
})

linker_type_arg = ""
if mg.platform() == "windows" then
	linker_type_arg = "-fuse-ld=lld"
elseif mg.platform() == "linux" then
	linker_type_arg = "-fuse-ld=ld"
end

local tests_exe = mg.project({
	name = "tests",
	type = mg.project_type.executable,
	sources = {"tests/**.cc"},
	includes = {"src"},
	compile_options = {"-g", "-O0", "-Wall", "-Wextra", "-Werror", "-Wno-dangling-else", "-fno-char8_t", "-nostdinc++", "--std=c++20", "-D_CRT_SECURE_NO_WARNINGS", "-fprofile-instr-generate", "-fcoverage-mapping", '-D"_MT"', '-D"_DLL"'},
	link_options = {linker_type_arg, "-g", "-fprofile-instr-generate", "-fcoverage-mapping", '-Xlinker /nodefaultlib:libcmt.lib', '-Xlinker /defaultlib:msvcrt.lib'},
	dependencies = {mincore}
})

if mg.need_generate() then
	mg.generate({tests_exe})
else
	return {project = mincore}
end
