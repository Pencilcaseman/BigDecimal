// This code was ported and modified for C++. The original code
// is BigInt.js, and was created by Leemon Baird (www.leemon.com)

#include <iostream>
#include <cstdint>
#include <string>
#include <chrono>
#include <cmath>
#include <librapid/librapid.hpp>

#include <mmintrin.h>

char DIGIT_STRING[96] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
                         'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                         'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
                         'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '_', '=', '!', '@', '#', '$',
                         '%', '^', '&', '*', '(', ')', '[', ']', '{', '}', '|', ';', ':', ',', '.', '<', '>',
                         '/', '?', '`', '~', ' ', '\'', '\'', '"', '+', '-'};


int64_t bpe = 0;           // Bits stored per array element
int64_t mask = 0;          // AND this with an array element to cut it down to bpe bits
int64_t radix = mask + 1;  // Equals 2^bpe. A single 1 bit to the left of the last bit mask
// TODO
// BigInt one;

void initializeBigInt() {
    for (bpe = 0; (((uint64_t) 1 << (bpe + 1)) > ((uint64_t) 1 << bpe)); bpe++); // bpe = number of bits in the mantissa
    bpe >>= 1; // bpe = number of bits in one element of the array representing the BigInt
    mask = ((int64_t) 1 << bpe) - (int64_t) 1; // AND this with an array element to cut it down to bpe bits
    radix = mask + 1;
    // TODO
    // one = BigInt(1, 1, 1)
}

class BigInt {
    using type = int64_t;

public:
    BigInt() = default;

    BigInt(type value, type bits = 64) {
        m_len = (bits / bpe) + 2;
        m_data = (type *) malloc(sizeof(type) * m_len); // new type[m_len];
        m_bits = m_len * bpe;
        copyInt(value);
    }

    BigInt(const BigInt &other) {
        m_len = other.m_len;
        m_data = new type[m_len];
        m_bits = other.m_bits;
        for (int64_t i = 0; i < m_len; ++i) m_data[i] = other.m_data[i];
    }

    BigInt &operator=(const BigInt &other) {
        if (this == &other)
            return *this;

        if (m_len != other.m_len) {
            m_bits = other.m_bits;
            m_len = other.m_len;
            delete[] m_data;
            m_data = new type[m_len];
        }

        for (int64_t i = 0; i < m_len; ++i) m_data[i] = other.m_data[i];

        return *this;
    }

    ~BigInt() {
        // delete[] m_data;
        free(m_data);
    }

    [[nodiscard]] bool isZero() const {
        if (m_data == nullptr) throw std::bad_alloc();

        for (int64_t i = 0; i < m_len; ++i)
            if (m_data[i])
                return false;
        return true;
    }

    [[nodiscard]] BigInt operator+(int64_t value) const {
        return add(value);
    }

    [[nodiscard]] BigInt operator+(const BigInt &value) const {
        return add(value);
    }

    [[nodiscard]] BigInt add(int64_t value) const {
        auto res = *this;
        res.addInplace(value);
        return res;
    }

    [[nodiscard]] BigInt add(const BigInt &value) const {
        auto res = *this;
        res.addInplace(value);
        return res;
    }

    void addInplace(int64_t value) {
        if (m_data == nullptr) throw std::bad_alloc();

        m_data[0] += value;
        int64_t carry = 0, b;
        for (int64_t i = 0; i < m_len; ++i) {
            carry += m_data[i];
            b = 0;
            if (carry < 0) {
                b = -(carry >> bpe);
                carry += b * radix;
            }
            m_data[i] = carry & mask;
            carry = (carry >> bpe) - b;
            if (!carry) return;
        }

        if (m_data[m_len - 1])
            reallocate(m_bits + bpe * 3);
    }

    inline void addInplace(const BigInt &value) {
        if (m_data == nullptr) throw std::bad_alloc();
        if (value.m_data == nullptr) throw std::bad_alloc();

        int64_t k = m_len < value.m_len ? m_len : value.m_len;
        int64_t c = 0;
        for (int64_t i = 0; i < k; ++i) {
            c += m_data[i] + value.m_data[i];
            m_data[i] = c & mask;
            c >>= bpe;
        }

        for (int64_t i = k; c && i < m_len; ++i) {
            c += m_data[i];
            m_data[i] = c & mask;
            c >>= bpe;
        }

        if (m_data[m_len - 1])
            reallocate(m_bits + bpe * 3);

        // if (m_len < value.m_len) throw std::runtime_error("NOPE!");
        // if (m_data == nullptr) throw std::bad_alloc();
        // if (value.m_data == nullptr) throw std::bad_alloc();

        // uint8_t carry = 0;
        // int64_t i = 0;

        // for (; i < value.m_len; ++i) {
        //     carry = _addcarry_u64(carry, m_data[i], value.m_data[i], m_data + i);
        // }

        // for (; carry; ++i) {
        //     carry = _addcarry_u64(carry, m_data[i], 0ull, m_data + i);
        // }

        // if (m_data[m_len - 1])
        //     reallocate(m_bits + bpe * 3);
    }

    [[nodiscard]] BigInt operator-(int64_t value) const {
        return sub(value);
    }

    [[nodiscard]] BigInt operator-(const BigInt &value) const {
        return sub(value);
    }

    [[nodiscard]] BigInt sub(int64_t value) const {
        BigInt res = *this;
        res.subInplace(value);
        return res;
    }

    [[nodiscard]] BigInt sub(const BigInt &value) const {
        BigInt res = *this;
        res.subInplace(value);
        return res;
    }

    void subInplace(int64_t value) {
        int64_t t = value;

        for (int64_t i = 0; i < m_len - 1; ++i) {
            m_data[i] -= t & mask;
            t >>= bpe;

            if (m_data[i] < 0) {
                m_data[i] += radix;
                --m_data[i + 1];
            }
        }

        if (m_data[m_len - 1] < 0)
            m_data[m_len - 1] += radix;
    }

    void subInplace(const BigInt &value) {
        int64_t k = m_len < value.m_len ? m_len : value.m_len;
        int64_t carry = 0;
        int64_t i;

        for (i = 0; i < k; ++i) {
            carry += m_data[i] - value.m_data[i];
            m_data[i] = carry & mask;
            carry >>= bpe;
        }

        for (; carry && i < m_len; ++i) {
            carry += m_data[i];
            m_data[i] = carry & mask;
            carry >>= bpe;
        }
    }

    void divQuotRem(BigInt &val, BigInt &quotient, BigInt &remainder) const {
        int64_t kx;
        int64_t ky; // ky is the number of elements in y, excluding leading zeros
        int64_t i, j, y1, y2, c, a, b;
        remainder = *this;

        for (ky = val.m_len; val.m_data[ky - 1] == 0; --ky);

        // Normalize: ensure the most significant element of y has its highest bit set
        b = val.m_data[ky - 1];
        for (a = 0; b; ++a)
            b >>= 1;
        a = bpe - a;
        val.leftShiftInplace(a);
        remainder.leftShiftInplace(a);

        for (kx = remainder.m_len; remainder.m_data[kx - 1] == 0 && kx > ky; --kx);

        quotient = BigInt(0, m_bits);
        while (!val.greaterShift(remainder, kx - ky)) {
            remainder.subShiftInplace(val, kx - ky);
            ++quotient.m_data[kx - ky];
        }

        for (i = kx - 1; i >= ky; --i) {
            if (remainder.m_data[i] == val.m_data[ky - 1]) quotient.m_data[i - ky] = mask;
            else quotient.m_data[i - ky] = (remainder.m_data[i] * radix + remainder.m_data[i - 1]) / val.m_data[ky - 1];

            for (;;) {
                y2 = (ky > 1 ? val.m_data[ky - 2] : 0) * quotient.m_data[i - ky];
                c = y2 >> bpe;
                y2 = y2 & mask;
                y1 = c + quotient.m_data[i - ky] * val.m_data[ky - 1];
                c = y1 >> bpe;
                y1 = y1 & mask;

                if (c == remainder.m_data[i] ?
                    (y1 == remainder.m_data[i - 1] ?
                     (y2 > (i > 1 ? remainder.m_data[i - 2] : 0)) :
                     y1 > remainder.m_data[i - 1]) : c > remainder.m_data[i]) {
                    --quotient.m_data[i - ky];
                } else {
                    break;
                }
            }

            // remainder = remainder - quotient[i - ky] * leftShift_(y, i - ky)
            remainder.linCombShiftInplace(val, -quotient.m_data[i - ky], i - ky);
            if (remainder.negative()) {
                remainder.addShiftInplace(val, i - ky);
                --quotient.m_data[i - ky];
            }
        }

        val.rightShiftInplace(a);
        remainder.rightShiftInplace(a);
    }

    /**
     * \rst
     *
     * Calculate x = floor(x / n) for BigInt x and integer n.
     * The remainder is given in `rem`
     *
     * \endrst
     */
    void divInt(int64_t value, int64_t &rem) {
        if (m_data == nullptr) throw std::bad_alloc();

        type s;
        rem = 0;
        for (int64_t i = m_len - 1; i >= 0; --i) {
            s = rem * radix + m_data[i];
            m_data[i] = s / value;
            rem = s % value;
        }
    }

    void leftShiftInplace(int64_t n) {
        // Locate the first set bit to check if there's room for the new value
        int64_t first;
        for (first = m_len - 1; !m_data[first] && first > 0; first--);
        if (bpe * ((m_len - 2) - first) < n)
            reallocate(m_bits + n + bpe * 3);

        int64_t i, k = n / bpe;

        if (k) {
            for (i = m_len - 1; i >= k; --i)
                m_data[i] = m_data[i - k];
            for (; i >= 0; --i)
                m_data[i] = 0;
            n %= bpe;
        }

        if (!n)
            return;
        for (i = m_len - 1; i > 0; --i)
            m_data[i] = mask & ((m_data[i] << n) | (m_data[i - 1] >> (bpe - n)));
        m_data[i] = mask & (m_data[i] << n);
    }

    void rightShiftInplace(int64_t n) {
        int64_t i, k = n / bpe;
        if (k) {
            for (i = 0; i < m_len - k; ++i)
                m_data[i] = m_data[i + k];
            for (; i < m_len; ++i)
                m_data[i] = 0;
            n %= bpe;
        }

        for (i = 0; i < m_len - 1; ++i)
            m_data[i] = mask & ((m_data[i + 1] << (bpe - n)) | (m_data[i] >> n));
        m_data[i] >>= n;
    }

    /**
     * \rst
     *
     * Is (x << (shift * bpe)) > y?
     *
     * \endrst
     */
    [[nodiscard]] bool greaterShift(const BigInt &other, int64_t shift) const {
        int64_t i, kx = m_len, ky = other.m_len;
        int64_t k = ((kx + shift) < ky) ? (kx + shift) : ky;

        for (i = ky - 1 - shift; i < kx && i >= 0; ++i)
            if (m_data[i] > 0)
                return true; // If there are non-zeros in *this to the left of the first column of y, then x is bigger
        for (i = kx - 1 + shift; i < ky; ++i)
            if (other.m_data[i] > 0)
                return false; // If there are non-zeros in y to the left of the first column of x, then x is not bigger
        for (i = k - 1; i >= shift; --i) {
            if (m_data[i - shift] > other.m_data[i]) return true;
            else if (m_data[i - shift] < other.m_data[i]) return false;
        }
        return false;
    }

    /**
     * \rst
     *
     * Calculate x = x + (y << (ys * bpe))
     *
     * \endrst
     */
    void addShiftInplace(const BigInt &other, int64_t ys) {
        int64_t i, c, k, kk;
        k = m_len < ys + other.m_len ? m_len : ys + other.m_len;
        kk = m_len;

        c = 0;
        for (i = ys; i < k; ++i) {
            c += m_data[i] + other.m_data[i - ys];
            m_data[i] = c & mask;
            c >>= bpe;
        }

        for (i = k; c && i < kk; ++i) {
            c += m_data[i];
            m_data[i] = c & mask;
            c >>= bpe;
        }

        if (m_data[m_len - 1])
            reallocate(m_bits + bpe * 3);
    }

    /**
     * \rst
     *
     * Calculate x = x - (y << (ys * bpe))
     *
     * \endrst
     */
    void subShiftInplace(const BigInt &other, int64_t ys) {
        int64_t i, c, k, kk;
        k = m_len < ys + other.m_len ? m_len : ys + other.m_len;
        kk = m_len;

        c = 0;
        for (i = ys; i < k; ++i) {
            c += m_data[i] - other.m_data[i - ys];
            m_data[i] = c & mask;
            c >>= bpe;
        }

        for (i = k; c && i < kk; ++i) {
            c += m_data[i];
            m_data[i] = c & mask;
            c >>= bpe;
        }
    }

    void linCombShiftInplace(const BigInt &other, int64_t b, int64_t ys) {
        int64_t i, c, k, kk;
        k = m_len < ys + other.m_len ? m_len : ys + other.m_len;
        kk = m_len;

        c = 0;
        for (i = ys; i < k; ++i) {
            c += m_data[i] + b * other.m_data[i - ys];
            m_data[i] = c & mask;
            c >>= bpe;
        }
        for (i = k; c && i < kk; ++i) {
            c += m_data[i];
            m_data[i] = c & mask;
            c >>= bpe;
        }
    }

    /**
     * \rst
     *
     * Is the value negative?
     *
     * \endrst
     */
    [[nodiscard]] bool negative() const {
        return ((m_data[m_len - 1] >> (bpe - 1)) & 1);
    }

    [[nodiscard]] std::string str(int64_t base = 10) const {
        if (m_data == nullptr) throw std::bad_alloc();

        std::string res;
        if (base < 2) { // Return array contents
            for (int64_t i = m_len - 1; i > 0; --i) {
                res += std::to_string(m_data[i]);
                res += ", ";
            }
            res += std::to_string(m_data[0]);
        } else {
            auto current = *this;
            BigInt tmp;
            int64_t r;
            while (!current.isZero()) {
                current.divInt(base, r);
                res.insert(res.begin(), DIGIT_STRING[r]);
            }
        }
        if (res.length() == 0) res = "0";
        return res;
    }

// private:
public:
    void reallocate(int64_t bits) {
        auto newLen = (bits / bpe) + 2;
        auto *newData = (type *) malloc(sizeof(type) * newLen); // new type[newLen];

        int64_t i;
        for (i = 0; i < m_len; ++i)
            newData[i] = m_data[i];

        for (; i < newLen; ++i)
            newData[i] = 0;

        std::swap(m_data, newData);
        // delete[] newData;
        free(newData);

        m_bits = bits;
        m_len = newLen;
    }

    /**
     * \rst
     *
     * Perform x=y on a BigInt x and a normal integer y
     *
     * \endrst
     */
    void copyInt(int64_t val) {
        int64_t tmp = val;
        for (int64_t i = 0; i < m_len; ++i) {
            m_data[i] = tmp & mask;
            tmp >>= bpe;
        }
    }

// private:
public:
    int64_t m_len = -1;
    int64_t m_bits = -1;
    type *m_data = nullptr;
};

std::ostream &operator<<(std::ostream &os, const BigInt &num) {
    return os << num.str();
}

int main() {
    std::cout << std::fixed;
    std::cout.precision(10);

    initializeBigInt();

    std::cout << "BPE: " << bpe << "\n";
    std::cout << "Radix: " << radix << "\n";

    BigInt q = BigInt(1ll << 63);
    std::cout << q << "\n";
    std::cout << q + q << "\n";
    std::cout << (q + q) + q << "\n";

    q = 1;
    for (int i = 0; i < 100; i++) {
        q = q + q;
        std::cout << q << "\n";
    }
    std::cout << q << "\n";

    {
        int64_t iters = 10000; // 1000;
        auto value = BigInt(1);
        std::cout << "Shifting\n";
        int64_t shift = 0;
        while (value.m_len < 100000) {
            value.leftShiftInplace(1 << 6);
            shift += 1 << 6;
        }
        std::cout << "Finished shifting. Shifted " << shift << " places\n";
        std::cout << "Length: " << value.m_len << "\n";

        // value.leftShiftInplace(100000);
        // auto denom = BigInt(1);
        // denom.leftShiftInplace(1000000 - 10);

        BigInt tmp = 0;
        auto start = (double) librapid::seconds();
        for (int64_t i = 0; i < iters; i++) {
            // value.leftShiftInplace(1000);
            // value.divQuotRem(denom, q, r);
            // value = value + value;
            // tmp = value + value;
            auto res = value + value;
        }
        auto end = (double) librapid::seconds();
        std::cout << "Elapsed: " << (end - start) << " s\n";
        std::cout << "Average: " << ((end - start) / (double) iters) * 1000000000 << " ns\n";

        // std::cout << value.str() << "\n";
        // std::cout << q.str() << "\n";

        // std::cout << tmp.str() << "\n";
    }

    return 0;
}
