#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

/**
 * Serialization backend must define these
 * IMPL_CEREAL_BACKEND()
 * IMPL_CEREAL_CTOR(type)
 *
 * Serialization backend may define these
 * IMPL_CEREAL_BEGIN(type)
 */

#undef PRIVATE_CEREAL_OVERRIDE
#ifdef CEREAL_CONFIG_OVERRIDE
#define PRIVATE_CEREAL_OVERRIDE() override
#else
#define PRIVATE_CEREAL_OVERRIDE()
#endif

#undef PRIVATE_CEREAL_VISIBILITY
#ifdef CEREAL_CONFIG_PUBLIC
#define PRIVATE_CEREAL_VISIBILITY() public:
#else
#define PRIVATE_CEREAL_VISIBILITY() private:
#endif

#undef PRIVATE_CEREAL_SET_VISIBILITY
#ifdef CEREAL_CONFIG_PROP_SET_PUBLIC
#define PRIVATE_CEREAL_SET_VISIBILITY() public:
#else
#define PRIVATE_CEREAL_SET_VISIBILITY() private:
#endif

#ifndef IMPL_CEREAL_BEGIN
#define IMPL_CEREAL_BEGIN(type)
#endif

/**
 * From here on, nothing changes based off what is defined, so use an include guard
 */

#ifndef CEREAL_H
#define CEREAL_H

#include <filesystem>
#include <utility>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <iostream>
#include <any>

/**
 * Classes and enums used by cereal, called by code generated by macros
 */

class cereal_base;

enum cereal_key_type {
    def,
    lowercase
};

class cereal_init_function {
    std::function<void(const std::any &instance, cereal_base *backend)> _func;
    bool _is_null = false;

public:
    cereal_init_function() {
        _is_null = true;
    }

    cereal_init_function(void (*func)()) {
        this->_func = [func](const std::any &i, cereal_base *b) { func(); };
    }

    template<class T>
    cereal_init_function(void (T::*func)()) {
        this->_func = [func](const std::any &i, cereal_base *b) {
            (std::any_cast<T *>(i)->*func)();
        };
    }

    template<class T, class BackendType>
    void call(T *instance, BackendType *backend) {
        if (!_is_null)
            _func(instance, (cereal_base *) backend);
    }
};

template<class ValueType>
class cereal_normalize_function {
    std::function<ValueType(const std::any &instance, cereal_base *backend, const ValueType &value)> _func;
    bool _is_null = false;

public:
    cereal_normalize_function() {
        _is_null = true;
    }

    cereal_normalize_function(ValueType (*func)(const ValueType &value)) {
        this->_func = [func](const std::any &i, const std::any &b, const ValueType &v) {
            return func(v);
        };
    }

    template<class T>
    cereal_normalize_function(void (T::*func)()) {
        this->_func = [func](const std::any &i) {
            std::any_cast<T *>(i)->*func();
        };
    }

    template<class BackendType>
    cereal_normalize_function(ValueType (*func)(BackendType *backend, const ValueType &value)) {
        this->_func = [func](const std::any &i, cereal_base *b, const ValueType &v) {
            return func((BackendType *) (b), v);
        };
    }

    template<class T, class BackendType>
    ValueType call(T *instance, BackendType *backend, const ValueType &value) {
        if (!_is_null)
            return _func(instance, (cereal_base *) backend, value);

        return ValueType();
    }

    [[nodiscard]] bool null() const {
        return _is_null;
    }
};

// Set by macro
class cereal_config {
public:
    bool always_load = false;
    bool always_save = false;
    cereal_key_type key_type = def;
    cereal_init_function init;
};

// Set by macro
template<class ValueType>
class cereal_prop_config {
public:
    bool required = false;
    cereal_normalize_function<ValueType> normalizer;
    ValueType default_value = ValueType();
};

class cereal_base {
public:
    [[nodiscard]] virtual bool loaded() const = 0;

    [[nodiscard]] virtual std::filesystem::path path() const = 0;

    virtual void load_file(const std::filesystem::path &path) = 0;

    virtual void save_file(const std::filesystem::path &path) = 0;
};

class cereal_prop_base {
public:
    virtual ~cereal_prop_base() = default;

    virtual cereal_prop_base *clone(cereal_base *backend, cereal_config *config) = 0;

    virtual void save() = 0;

    virtual void load(const std::any &instance) = 0;

    virtual void reset(const std::any &instance) = 0;
};

template<class T, class BackendType, class ValueType>
class cereal_prop : public cereal_prop_base {
    BackendType *_backend;
    cereal_config *_cereal_config;
    cereal_prop_config<ValueType> _prop_config;

    std::string _key;
    ValueType _current_value = ValueType();
    bool _changed = false;

public:
    cereal_prop(const std::string &name,
                BackendType *backend,
                cereal_config *cereal_config,
                cereal_prop_config<ValueType> prop_config)
            : _backend(backend), _cereal_config(cereal_config), _prop_config(std::move(prop_config)) {
        _key = name;
        _changed = false;
        if (cereal_config->key_type == lowercase)
            std::transform(_key.begin(), _key.end(), _key.begin(), ::tolower);
    }

    cereal_prop_base *clone(cereal_base *cereal_backend, cereal_config *config) override {
        auto copy = new cereal_prop<T, BackendType, ValueType>(*this);
        copy->_backend = (BackendType *) cereal_backend;
        copy->_cereal_config = config;
        return copy;
    }

    void save() override {
        if (_changed)
            _backend->template save_value<ValueType>(_key, _current_value);
        _changed = false;
    }

    void load(const std::any &instance) override {
        if (!_backend->value_exists(_key)) {
            if (_prop_config.required)
                throw std::runtime_error(_key + " does not exist!");
            else
                _current_value = normalize(std::any_cast<T *>(instance), _prop_config.default_value);
            _changed = false;
            return;
        }

        _current_value = normalize(std::any_cast<T *>(instance), _backend->template load_value<ValueType>(_key));
        _changed = false;
    }

    void reset(const std::any &instance) override {
        load(instance);
    }

    const ValueType &get(T *instance) {
        if (_changed)
            return _current_value;

        if (_cereal_config->always_load)
            load(instance);

        return _current_value;
    }

    void set(T *instance, const ValueType &value) {
        _changed = true;
        _current_value = normalize(instance, value);
        if (_cereal_config->always_save)
            save();
    }

private:
    ValueType normalize(T *instance, const ValueType &value) const {
        if (!_prop_config.normalizer.null())
            return ((cereal_normalize_function<ValueType>) _prop_config.normalizer).call(
                    (T *) instance,
                    (BackendType *) _backend,
                    (const ValueType &) value);
        return value;
    }
};

template<class T, class BackendType>
class cereal : public cereal_base {
    std::unordered_map<std::string, cereal_prop_base *> _props;
    cereal_config _config;
    std::filesystem::path _file_path;
    bool _has_file_path = false;
    bool _props_loaded = false;

public:
    explicit cereal(const cereal_config &config) {
        this->_config = config;
    }

    cereal(const cereal &other) {
        copy(other);
    }

    cereal &operator=(const cereal &other) {
        copy(other);
        return *this;
    }

    ~cereal() {
        for (const auto &prop : _props)
            delete prop.second;
    }

    [[nodiscard]] bool loaded() const override {
        return _props_loaded;
    }

    [[nodiscard]] std::filesystem::path path() const override {
        if (!_has_file_path)
            throw std::runtime_error("This cereal object does not have a path!");
        return _file_path;
    }

    template<class ValueType>
    void add_prop(const std::string &name, const cereal_prop_config<ValueType> &prop_config) {
        auto *prop = new cereal_prop<T, BackendType, ValueType>(
                name,
                (BackendType *) this,
                &this->_config,
                prop_config
        );

        _props[name] = prop;
    }

    void save_props() {
        for (const auto &prop : _props)
            prop.second->save();

        if (_has_file_path)
            save_file(path());
    }

    void load_props(T *instance) {
        if (_has_file_path)
            load_file(path());

        for (const auto &prop : _props)
            prop.second->load(instance);

        _config.init.call(instance, this);
        _props_loaded = true;
    }

    void reset_props(T *instance) {
        for (const auto &prop : _props)
            prop.second->reset(instance);
    }

    template<class ValueType>
    const ValueType &get_value(const T *instance, const std::string &key) const {
        return ((cereal_prop<T, BackendType, ValueType> *) _props.at(key))->get((T *) instance);
    }

    template<class ValueType>
    void set_value(T *instance, const std::string &key, const ValueType &value) {
        ((cereal_prop<T, BackendType, ValueType> *) _props.at(key))->set(instance, value);
    }

protected:
    void set_path(const std::filesystem::path &path) {
        _file_path = path;
        _has_file_path = true;
    }

private:
    void copy(const cereal &other) {
        _config = other._config;
        _props.clear();
        for (const auto &other_prop : other._props)
            _props[other_prop.first] = other_prop.second->clone((BackendType *) this, &_config);
        _props_loaded = other._props_loaded;
        _file_path = other._file_path;
        _has_file_path = other._has_file_path;
    }
};

/**
 * Macros to generate save/get/set/etc. functions for properties
 */

#define CEREAL_BEGIN_CUSTOM(type, custom_config)                                            \
        IMPL_CEREAL_BACKEND()<type> _cereal = IMPL_CEREAL_BACKEND()<type>((custom_config)); \
    public:                                                                                 \
        IMPL_CEREAL_CTOR(type)                                                              \
        void load() { _cereal.load_props(this); }                                           \
        bool loaded() const { return _cereal.loaded(); }                                    \
        std::filesystem::path path() const { return _cereal.path(); }                       \
    PRIVATE_CEREAL_VISIBILITY()                                                             \
        void save() { _cereal.save_props(); }                                               \
        void reset() { _cereal.reset_props(this); }                                         \
    IMPL_CEREAL_BEGIN(type)                                                                 \
    private:

#define CEREAL_BEGIN(type) \
    CEREAL_BEGIN_CUSTOM(type, cereal_config { })

#define CEREAL_PROP_CUSTOM(name, type, custom_config)                  \
        bool _cereal_##name##_initialized = [&]() {                    \
            _cereal.add_prop(#name, custom_config);                    \
            return true;                                               \
        }();                                                           \
    public:                                                            \
        const type &name() const PRIVATE_CEREAL_OVERRIDE() {           \
            return _cereal.get_value<type>(this, #name);               \
        }                                                              \
    PRIVATE_CEREAL_SET_VISIBILITY()                                    \
        void set_##name(const type &value) PRIVATE_CEREAL_OVERRIDE() { \
            _cereal.set_value(this, #name, value);                     \
        }                                                              \
    private:

#define CEREAL_PROP_REQUIRED(name, type) \
    CEREAL_PROP_CUSTOM(name, type, CerealPropConfig<type> { .required = true })

#define CEREAL_NORM_PROP_REQUIRED(name, type, norm)            \
    CEREAL_PROP_CUSTOM(name, type, (cereal_prop_config<type> { \
        .required = true,                                      \
        .normalizer = (norm)                                   \
    }))

#define CEREAL_PROP_DEFAULT(name, type, value) \
    CEREAL_PROP_CUSTOM(name, type, CerealPropConfig<type> { .defaultValue = (value) })

#define CEREAL_NORM_PROP_DEFAULT(name, type, value, norm)      \
    CEREAL_PROP_CUSTOM(name, type, (cereal_prop_config<type> { \
        .defaultValue = (value),                               \
        .normalizer = CEREAL_NORM_FUNC(norm, type)             \
    }))

#define CEREAL_PROP(name, type) \
    CEREAL_PROP_CUSTOM(name, type, cereal_prop_config<type> {  })

#define CEREAL_GET(name, type)                               \
        type _cereal_##name = type();                        \
    public:                                                  \
        const type &name() const PRIVATE_CEREAL_OVERRIDE() { \
            return _cereal_##name;                           \
        }                                                    \
    private:

#define CEREAL_GET_PRIVATE_SET(name, type)                               \
        type _cereal_##name = type();                                    \
    public:                                                              \
        const type &name() const PRIVATE_CEREAL_OVERRIDE(){              \
            return _cereal_##name;                                       \
        }                                                                \
    private:                                                             \
        void set_##name##(const type &value) PRIVATE_CEREAL_OVERRIDE() { \
            _cereal_##name = value;                                      \
        }

#endif
#pragma clang diagnostic pop