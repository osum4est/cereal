#undef IMPL_CEREAL_BACKEND
#define IMPL_CEREAL_BACKEND() cereal_json

#undef IMPL_CEREAL_CTOR
#define IMPL_CEREAL_CTOR(type)                                              \
    type() = default;                                                       \
    explicit type(const std::string &json_str) { _cereal.load(this, json_str); } \
    explicit type(const nlohmann::json &json) { _cereal.load(this, json); } \
    explicit type(const std::filesystem::path &path) { _cereal.load(this, path); }

#undef IMPL_CEREAL_BEGIN
#define IMPL_CEREAL_BEGIN(type)                                   \
    public:                                                       \
        friend void from_json(const nlohmann::json &j, type &t) { \
            t._cereal.load(&t, j);                                \
        }                                                         \
        friend void to_json(nlohmann::json &j, const type &t) {   \
            j = t._cereal.json();                                 \
        }

#ifndef CEREAL_JSON_H
#define CEREAL_JSON_H

#include <nlohmann/json.hpp>
#include <cereal/cereal.h>
#include <fstream>

template<class T>
class cereal_json : public cereal<T, cereal_json<T>> {
    nlohmann::json _cereal_json;

public:
    explicit cereal_json(const cereal_config &config) : cereal<T, cereal_json<T>>(config) { }

    [[nodiscard]] nlohmann::json json() const {
        return _cereal_json;
    }

    void load(T *instance, const std::string &json_str) {
        _cereal_json = nlohmann::json::parse(json_str);
        this->load_props(instance);
    }

    void load(T *instance, const nlohmann::json &json) {
        _cereal_json = json;
        this->load_props(instance);
    }

    void load(T *instance, const std::filesystem::path &path) {
        this->set_path(path);
        this->load_props(instance);
    }

    void load_file(const std::filesystem::path &path) {
        _cereal_json = nlohmann::json();
        std::ifstream file(path);
        if (file)
            file >> _cereal_json;
    }

    void save_file(const std::filesystem::path &path) {
        std::ofstream file(path);
        if (file)
            file << _cereal_json;
    }

    template<class ValueType>
    ValueType load_value(const std::string &key) {
        return _cereal_json[key].template get<ValueType>();
    }

    template<class ValueType>
    void save_value(const std::string &key, const ValueType &value) {
        _cereal_json[key] = value;
    }

    bool value_exists(const std::string &key) {
        return _cereal_json.contains(key);
    }
};

#endif

#include <cereal/cereal_reset.h>
