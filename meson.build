project('FrameRateConverter', 'cpp',
    default_options: ['buildtype=release', 'b_ndebug=if-release', 'cpp_std=c++17'],
  )

sources = [
  'Src/Common/ContinuousMaskBase.cpp',
  'Src/Common/ConvertFpsLimitBase.cpp',
  'Src/Common/StripeMaskBase.cpp',
  'Src/Common/merge.cpp',
  # 'Src/Common/merge_avx2.cpp',
  'Src/Common/ContinuousMaskBase.cpp',
  'Src/Environments/cpufeatures.cpp',
  'Src/Environments/instrset_detect.cpp',
  'Src/VapourSynth/ContinuousMaskVpy.cpp',
  'Src/VapourSynth/ConvertFpsLimitVpy.cpp',
  'Src/VapourSynth/InitVpy.cpp',
  'Src/VapourSynth/StripeMaskVpy.cpp',
  # 'Src/Avisynth/ContinuousMaskAvs.cpp',
  # 'Src/Avisynth/ConvertFpsLimitAvs.cpp',
  # 'Src/Avisynth/InitAvs.cpp',
  # 'Src/Avisynth/StripeMaskAvs.cpp',
  # 'Src/Avisynth/conditional.cpp',
  # 'Src/Avisynth/conditional_functions.cpp',
]

vapoursynth_dep = dependency('vapoursynth', version: '>=55').partial_dependency(compile_args : true, includes : true)

deps = [vapoursynth_dep]
libs = []

if host_machine.cpu_family().startswith('x86')
    add_project_arguments('-mfpmath=sse', '-msse2', language : ['c', 'cpp'])

    libs += static_library('merge_avx2', 'Src/Common/merge_avx2.cpp',
                # dependencies: [m_dep],
                cpp_args: ['-mavx2', '-mfma'],
                pic: true,
                # include_directories: includedirs
            )
endif

shared_module('framerateconverter', sources,
  dependencies : deps,
  link_with : libs,
  install : true,
  install_dir : join_paths(vapoursynth_dep.get_pkgconfig_variable('libdir'), 'vapoursynth'),
  gnu_symbol_visibility : 'hidden'
)
