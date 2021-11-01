#undef IMPL_CEREAL_BACKEND
#define IMPL_CEREAL_BACKEND() cereal_yaml

#undef IMPL_CEREAL_CTOR
#define IMPL_CEREAL_CTOR(type)                                          \
    type() = default;                                                   \
    explicit type(const YAML::Node &yaml) { _cereal.load(this, yaml); } \
    explicit type(const std::filesystem::path &path) { _cereal.load(this, path); }

#undef IMPL_CEREAL_BEGIN
#define IMPL_CEREAL_BEGIN(type) \
    friend YAML::convert<type>;

#undef CEREAL_YAML_FINALIZE
#define CEREAL_YAML_FINALIZE(type)                             \
    namespace YAML {                                           \
        template<>                                             \
        struct convert<type> {                                 \
            static YAML::Node encode(const type &t) {          \
                return t._cereal.yaml();                       \
            }                                                  \
                                                               \
            static bool decode(const YAML::Node &j, type &t) { \
                t._cereal.load(&t, j);                         \
                return true;                                   \
            }                                                  \
        };                                                     \
    }

#ifndef CEREAL_YAML_H
#define CEREAL_YAML_H

#include <yaml-cpp/yaml.h>
#include <cereal/cereal.h>
#include <fstream>

template<class T>
class cereal_yaml: public cereal<T, cereal_yaml<T>> {
    YAML::Node _cereal_yaml;

public:
    explicit cereal_yaml(const cereal_config &config) : cereal<T, cereal_yaml<T>>(config) { }

    [[nodiscard]] YAML::Node yaml() const {
        return _cereal_yaml;
    }

    void load(T *instance, const YAML::Node &yaml) {
        _cereal_yaml = yaml;
        this->load_props(instance);
    }

    void load(T *instance, const std::filesystem::path &path) {
        this->set_path(path);
        this->load_props(instance);
    }

    void load_file(const std::filesystem::path &path) {
        _cereal_yaml = YAML::LoadFile(path);
    }

    void save_file(const std::filesystem::path &path) {
        std::ofstream file(path);
        if (file)
            file << _cereal_yaml;
    }

    template<class ValueType>
    ValueType load_value(const std::string &key) {
        return _cereal_yaml[key].template as<ValueType>();
    }

    template<class ValueType>
    void save_value(const std::string &key, const ValueType &value) {
        _cereal_yaml[key] = value;
    }

    bool value_exists(const std::string &key) {
        return _cereal_yaml[key].IsDefined();
    }
};

#endif

#include <cereal/cereal_reset.h>
