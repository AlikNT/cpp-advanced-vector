#pragma once
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

template<typename T>
class RawMemory {
public:
    RawMemory() = default;
    explicit RawMemory(size_t capacity);
    RawMemory(const RawMemory &) = delete;
    RawMemory &operator=(const RawMemory &rhs) = delete;
    RawMemory(RawMemory &&other) noexcept;
    RawMemory &operator=(RawMemory &&rhs) noexcept;
    ~RawMemory();
    T *operator+(size_t offset) noexcept;
    const T *operator+(size_t offset) const noexcept;
    const T &operator[](size_t index) const noexcept;
    T &operator[](size_t index) noexcept;
    void Swap(RawMemory &other) noexcept;
    const T *GetAddress() const noexcept;
    T *GetAddress() noexcept;
    size_t Capacity() const;

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T *Allocate(size_t n);

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T *buf) noexcept;

    T *buffer_ = nullptr;
    size_t capacity_ = 0;
};



template<typename T>
class Vector {
public:
    using iterator = T *;
    using const_iterator = const T *;

    Vector() = default;

    ~Vector();

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;


    template<typename... Args>
    iterator Emplace(const_iterator pos, Args &&... args);

    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>);
    iterator Insert(const_iterator pos, const T &value);
    iterator Insert(const_iterator pos, T &&value);
    explicit Vector(size_t size);
    Vector(const Vector &other);
    Vector(Vector &&other) noexcept;
    Vector &operator=(const Vector &rhs);
    Vector &operator=(Vector &&rhs) noexcept;
    void Swap(Vector &other) noexcept;
    void Resize(size_t new_size);
    void PushBack(const T &value);
    void PushBack(T &&value);

    template<typename... Args>
    T &EmplaceBack(Args &&... args);

    void PopBack() noexcept;
    void Reserve(size_t new_capacity);
    size_t Size() const noexcept;
    size_t Capacity() const noexcept;
    const T &operator[](size_t index) const noexcept;
    T &operator[](size_t index) noexcept;

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    template<typename... Args>
    void EmplaceInNewAlloc(size_t pos_index, Args &&... args);

    template<typename... Args>
    void EmplaceInCurrentAlloc(size_t pos_index, Args &&... args);
};

template<typename T>
RawMemory<T>::RawMemory(size_t capacity): buffer_(Allocate(capacity))
                                          , capacity_(capacity) {
}

template<typename T>
RawMemory<T>::RawMemory(RawMemory &&other) noexcept: buffer_(std::move(other.buffer_))
                                                     , capacity_(other.capacity_) {
    other.buffer_ = nullptr;
    other.capacity_ = 0;
}

template<typename T>
RawMemory<T> & RawMemory<T>::operator=(RawMemory &&rhs) noexcept {
    if (this != &rhs) {
        Deallocate(buffer_);
        buffer_ = std::move(rhs.buffer_);
        capacity_ = rhs.capacity_;
        rhs.buffer_ = nullptr;
        rhs.capacity_ = 0;
    }
    return *this;
}

template<typename T>
RawMemory<T>::~RawMemory() {
    Deallocate(buffer_);
}

template<typename T>
T * RawMemory<T>::operator+(size_t offset) noexcept {
    // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
    assert(offset <= capacity_);
    return buffer_ + offset;
}

template<typename T>
const T * RawMemory<T>::operator+(size_t offset) const noexcept {
    return const_cast<RawMemory &>(*this) + offset;
}

template<typename T>
const T & RawMemory<T>::operator[](size_t index) const noexcept {
    return const_cast<RawMemory &>(*this)[index];
}

template<typename T>
T & RawMemory<T>::operator[](size_t index) noexcept {
    assert(index < capacity_);
    return buffer_[index];
}

template<typename T>
void RawMemory<T>::Swap(RawMemory &other) noexcept {
    std::swap(buffer_, other.buffer_);
    std::swap(capacity_, other.capacity_);
}

template<typename T>
const T * RawMemory<T>::GetAddress() const noexcept {
    return buffer_;
}

template<typename T>
T * RawMemory<T>::GetAddress() noexcept {
    return buffer_;
}

template<typename T>
size_t RawMemory<T>::Capacity() const {
    return capacity_;
}

template<typename T>
T * RawMemory<T>::Allocate(size_t n) {
    return n != 0 ? static_cast<T *>(operator new(n * sizeof(T))) : nullptr;
}

template<typename T>
void RawMemory<T>::Deallocate(T *buf) noexcept {
    operator delete(buf);
}

template<typename T>
typename Vector<T>::iterator Vector<T>::begin() noexcept {
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::iterator Vector<T>::end() noexcept {
    return data_.GetAddress() + size_;
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::begin() const noexcept {
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::end() const noexcept {
    return data_.GetAddress() + size_;
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cend() const noexcept {
    return data_.GetAddress() + size_;
}

template<typename T>
template<typename ... Args>
typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args &&...args) {
    size_t pos_index = 0;
    if (pos == data_ + size_) {
        pos_index = size_;
    } else {
        pos_index = pos - begin();
    }
    if (size_ == data_.Capacity()) {
        EmplaceInNewAlloc(pos_index, std::forward<Args>(args)...);
    } else {
        EmplaceInCurrentAlloc(pos_index, std::forward<Args>(args)...);
    }
    ++size_;
    return data_.GetAddress() + pos_index;
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
    size_t pos_index = pos - cbegin();
    iterator non_const_pos = begin() + pos_index;

    if (pos_index < size_ - 1) {
        std::move(non_const_pos + 1, end(), non_const_pos);
    }

    std::destroy_at(data_ + size_ - 1);
    --size_;

    return begin() + pos_index;
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, const T &value) {
    return Emplace(pos, value);
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, T &&value) {
    return Emplace(pos, std::move(value));
}

template<typename T>
Vector<T>::Vector(size_t size)
    : data_(size)
    , size_(size) {
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}

template<typename T>
Vector<T>::Vector(const Vector &other)
    : data_(other.size_)
    , size_(other.size_) {
    std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
}

template<typename T>
Vector<T>::Vector(Vector &&other) noexcept
    : data_(std::move(other.data_))
    , size_(other.size_) {
    other.size_ = 0;
}

template<typename T>
Vector<T> & Vector<T>::operator=(const Vector &rhs) {
    if (this != &rhs) {
        if (rhs.size_ > data_.Capacity()) {
            Vector tmp(rhs);
            Swap(tmp);
        } else {
            if (size_ > rhs.size_) {
                std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
            }
            std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + std::min(size_, rhs.size_),
                      data_.GetAddress());
            if (size_ < rhs.size_) {
                std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_,
                                          data_.GetAddress() + size_);
            }
            size_ = rhs.size_;
        }
    }
    return *this;
}

template<typename T>
Vector<T> & Vector<T>::operator=(Vector &&rhs) noexcept {
    if (this != &rhs) {
        data_ = std::move(rhs.data_);
        size_ = rhs.size_;
        rhs.size_ = 0;
    }
    return *this;
}

template<typename T>
void Vector<T>::Swap(Vector &other) noexcept {
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
}

template<typename T>
void Vector<T>::Resize(size_t new_size) {
    if (new_size > size_) {
        Reserve(new_size);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        } else {
            try {
                std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            } catch (...) {
                std::destroy_n(data_.GetAddress() + size_, new_size - size_);
                throw;
            }
        }
    } else {
        std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
    }
    size_ = new_size;
}

template<typename T>
void Vector<T>::PushBack(const T &value) {
    EmplaceBack(value);
}

template<typename T>
void Vector<T>::PushBack(T &&value) {
    EmplaceBack(std::move(value));
}

template<typename T>
template<typename ... Args>
T & Vector<T>::EmplaceBack(Args &&...args) {
    return *Emplace(end(), std::forward<Args>(args)...);
}

template<typename T>
void Vector<T>::PopBack() noexcept {
    std::destroy_n(data_.GetAddress() + size_ - 1, 1);
    --size_;
}

template<typename T>
Vector<T>::~Vector() {
    std::destroy_n(data_.GetAddress(), size_);
}

template<typename T>
void Vector<T>::Reserve(size_t new_capacity) {
    if (new_capacity <= data_.Capacity()) {
        return;
    }
    RawMemory<T> new_data(new_capacity);
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
    } else {
        std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}

template<typename T>
size_t Vector<T>::Size() const noexcept {
    return size_;
}

template<typename T>
size_t Vector<T>::Capacity() const noexcept {
    return data_.Capacity();
}

template<typename T>
const T & Vector<T>::operator[](size_t index) const noexcept {
    return const_cast<Vector &>(*this)[index];
}

template<typename T>
T & Vector<T>::operator[](size_t index) noexcept {
    assert(index < size_);
    return data_[index];
}

template<typename T>
template<typename ... Args>
void Vector<T>::EmplaceInNewAlloc(size_t pos_index, Args &&...args) {
    RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
    new(new_data + pos_index) T(std::forward<Args>(args)...);
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(begin(), pos_index, new_data.GetAddress());
        std::uninitialized_move_n(begin() + pos_index, size_ - pos_index, new_data.GetAddress() + pos_index + 1);
    } else {
        std::uninitialized_copy_n(begin(), pos_index, new_data.GetAddress());
        std::uninitialized_copy_n(begin() + pos_index, size_ - pos_index, new_data.GetAddress() + pos_index + 1);
    }
    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}

template<typename T>
template<typename ... Args>
void Vector<T>::EmplaceInCurrentAlloc(size_t pos_index, Args &&...args) {
    if (pos_index < size_) {
        T temp(std::forward<Args>(args)...);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            new (data_ + size_) T(std::move(*(data_ + size_ - 1)));
        } else {
            new (data_ + size_) T(*(data_ + size_ - 1));
        }
        std::move_backward(begin() + pos_index, end() - 1, end());
        data_[pos_index] = std::move(temp);
    } else {
        new (data_ + size_) T(std::forward<Args>(args)...);
    }
}
