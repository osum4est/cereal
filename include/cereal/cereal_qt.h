#ifdef QT_VERSION

#undef IMPL_CEREAL_BACKEND
#define IMPL_CEREAL_BACKEND() cereal_qt

#undef IMPL_CEREAL_CTOR
#define IMPL_CEREAL_CTOR(type) \
    type() { _cereal.load(this); }

#undef IMPL_CEREAL_BEGIN

#ifndef CEREAL_QT_H
#define CEREAL_QT_H

#include <cereal/cereal.h>

template<class T>
class cereal_qt : public cereal<T, cereal_qt<T>> {
private:
    QSettings cereal_qt_settings;

public:
    explicit cereal_qt(const cereal_config &config) : cereal<T, cereal_qt<T>>(config) {}

    void load(T *instance) {
        this->load_props(instance);
    }

    template<class ValueType>
    ValueType load_value(const std::string &key) {
        return cereal_qt_settings.value(QString::fromStdString(key)).template value<ValueType>();
    }

    template<class ValueType>
    void save_value(const std::string &key, const ValueType &value) {
        cereal_qt_settings.setValue(QString::fromStdString(key), QVariant::fromValue(value));
    }

    bool value_exists(const std::string &key) {
        return cereal_qt_settings.contains(QString::fromStdString(key));
    }
};

#endif
#endif

#include <cereal/cereal_reset.h>
