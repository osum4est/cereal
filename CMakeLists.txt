set(TARGET_NAME cereal)

add_library(${TARGET_NAME}
        include/cereal/cereal.h
        include/cereal/cereal_json.h
        include/cereal/cereal_yaml.h
        include/cereal/cereal_qt.h
        include/cereal/cereal_reset.h)

set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)