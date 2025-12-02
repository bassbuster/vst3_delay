
cmake_minimum_required(VERSION 3.25.0)

project(smtg-vst3-megalay
    VERSION 2.0.0.0
    DESCRIPTION "Steinberg VST 3 Bassbuster Megalay"
)

set(target megalay)

smtg_add_vst3plugin(${target}
    source/entry.cpp
    source/megalaycids.h
    source/megalaycontroller.cpp
    source/megalaycontroller.h
    source/megalayparamids.h
    source/megalayprocessor.cpp
    source/megalayprocessor.h
    source/version.h
    #${SDK_ROOT}/public.sdk/source/vst/utility/test/ringbuffertest.cpp
    #${SDK_ROOT}/public.sdk/source/vst/utility/test/versionparsertest.cpp
)

#smtg_target_setup_as_vst3_example(${target})

set_target_properties(${target}
    PROPERTIES
        ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER}
)

target_compile_features(${target}
    PUBLIC
        cxx_std_17
)

smtg_target_setup_options(${target}
    BUNDLE_IDENTIFIER "com.steinberg.vst3.${target}"
    COMPANY_NAME "Steinberg Media Technologies"
)

