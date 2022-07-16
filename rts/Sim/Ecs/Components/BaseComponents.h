#ifndef BASE_COMPONENTS_H__
#define BASE_COMPONENTS_H__

template<class T>
struct BasicComponentType {
    T value = 0;
};

#define ALIAS_COMPONENT(Component, T) struct Component : public BasicComponentType<T> {};

#endif