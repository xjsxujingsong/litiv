
// This file is part of the LITIV framework; visit the original repository at
// https://github.com/plstcharles/litiv for more information.
//
// Copyright 2015 Pierre-Luc St-Charles; pierre-luc.st-charles<at>polymtl.ca
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "litiv/utils/parallel.hpp"

namespace lv {

    /// computes the L1 distance between two integer values; returns an unsigned type twice as large as the input
    template<typename T, typename=std::enable_if_t<std::is_integral<T>::value>>
    inline auto L1dist(T a, T b) {
        static_assert(!std::is_same<T,bool>::value,"L1dist not specialized for boolean types");
        return (std::make_unsigned_t<T>)std::abs(((std::make_signed_t<typename lv::get_bigger_integer<T>::type>)a)-b);
    }

#if USE_SIGNEXT_SHIFT_TRICK
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-aliasing"
#elif (defined(__GNUC__) || defined(__GNUG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif //defined(...GCC)

    /// computes the L1 distance between two floating point values (with bit trick)
    template<typename T, typename=std::enable_if_t<std::is_floating_point<T>::value>>
    inline T L1dist(T a, T b) {
        static_assert(sizeof(T)==4 || sizeof(T)==8,"L1dist not defined for long double or non-ieee fp types");
        using Tint = std::conditional_t<sizeof(T)==4,int32_t,int64_t>;
        T fDiff = a-b;
        Tint MAY_ALIAS nCast = reinterpret_cast<Tint&>(fDiff);
        nCast &= std::numeric_limits<Tint>::max();
        return reinterpret_cast<T&>(nCast);
    }

#if defined(__clang__)
#pragma clang diagnostic pop
#elif (defined(__GNUC__) || defined(__GNUG__))
#pragma GCC diagnostic pop
#endif //defined(...GCC)
#else //!USE_SIGNEXT_SHIFT_TRICK

    /// computes the L1 distance between two floating point values (without bit trick)
    template<typename T, typename=std::enable_if_t<std::is_floating_point<T>::value>>
    inline T L1dist(T a, T b) {
        return std::abs(a-b);
    }

#endif //!USE_SIGNEXT_SHIFT_TRICK

    /// computes the L1 distance between two generic arrays
    template<size_t nChannels, typename Tin, typename Tout=decltype(L1dist(Tin(),Tin()))>
    inline Tout L1dist(const Tin* const a, const Tin* const b) {
        Tout tResult = 0;
        for(size_t c=0; c<nChannels; ++c)
            tResult += (Tout)L1dist(a[c],b[c]);
        return tResult;
    }

    /// computes the L1 distance between two generic arrays
    template<size_t nChannels, typename Tin, typename Tout=decltype(L1dist(Tin(),Tin()))>
    inline Tout L1dist(const std::array<Tin,nChannels>& a, const std::array<Tin,nChannels>& b) {
        return L1dist<nChannels,Tin,Tout>(a.data(),b.data());
    }

    /// computes the L1 distance between two generic arrays
    template<size_t nChannels, typename Tin, typename Tout=decltype(L1dist(Tin(),Tin()))>
    inline Tout L1dist(const std::array<Tin,nChannels>& a, const Tin* const b) {
        return L1dist<nChannels,Tin,Tout>(a.data(),b);
    }

    /// computes the L1 distance between two generic arrays
    template<size_t nChannels, typename Tin, typename Tout=decltype(L1dist(Tin(),Tin()))>
    inline Tout L1dist(const Tin* const a, const std::array<Tin,nChannels>& b) {
        return L1dist<nChannels,Tin,Tout>(a,b.data());
    }

    /// computes the L1 distance between two generic arrays
    template<size_t nChannels, typename Tin, typename Tout=std::conditional_t<std::is_integral<Tin>::value,size_t,float>>
    inline Tout L1dist(const Tin* const a, const Tin* const b, size_t nElements, const uint8_t* m=nullptr) {
        Tout tResult = 0;
        const size_t nTotElements = nElements*nChannels;
        if(m) {
            for(size_t n=0,i=0; n<nTotElements; n+=nChannels,++i)
                if(m[i])
                    tResult += L1dist<nChannels,Tin,Tout>(a+n,b+n);
        }
        else {
            for(size_t n=0; n<nTotElements; n+=nChannels)
                tResult += L1dist<nChannels,Tin,Tout>(a+n,b+n);
        }
        return tResult;
    }

    /// computes the L1 distance between two generic arrays
    template<typename Tin, typename Tout=std::conditional_t<std::is_integral<Tin>::value,size_t,float>>
    inline Tout L1dist(const Tin* const a, const Tin* const b, size_t nElements, size_t nChannels, const uint8_t* m=nullptr) {
        lvAssert_(nChannels>0 && nChannels<=4,"untemplated distance function only defined for 1 to 4 channels");
        switch(nChannels) {
            case 1: return L1dist<1,Tin,Tout>(a,b,nElements,m);
            case 2: return L1dist<2,Tin,Tout>(a,b,nElements,m);
            case 3: return L1dist<3,Tin,Tout>(a,b,nElements,m);
            case 4: return L1dist<4,Tin,Tout>(a,b,nElements,m);
            default: return 0;
        }
    }

#if USE_CVCORE_WITH_UTILS

    /// computes the L1 distance between two opencv vectors
    template<int nChannels, typename Tin, typename Tout=decltype(L1dist(Tin(),Tin()))>
    inline Tout L1dist(const cv::Vec<Tin,nChannels>& a, const cv::Vec<Tin,nChannels>& b) {
        Tin a_array[nChannels], b_array[nChannels];
        for(int c=0; c<nChannels; ++c) {
            a_array[c] = a[c];
            b_array[c] = b[c];
        }
        return L1dist<nChannels,Tin,Tout>(a_array,b_array);
    }

#endif //USE_CVCORE_WITH_UTILS

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// computes the squared L2 distance between two generic variables
    template<typename T>
    inline auto L2sqrdist(T a, T b) -> decltype(L1dist(a,b)) {
        auto gResult = L1dist(a,b);
        return gResult*gResult;
    }

    /// computes the squared L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline auto L2sqrdist(const T* const a, const T* const b) -> decltype(L2sqrdist(*a,*b)) {
        static_assert(nChannels>0,"vectors should have at least one channel");
        decltype(L2sqrdist(*a,*b)) gResult = 0;
        for(size_t c=0; c<nChannels; ++c)
            gResult += L2sqrdist(a[c],b[c]);
        return gResult;
    }

    /// computes the squared L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline auto L2sqrdist(const std::array<T,nChannels>& a, const std::array<T,nChannels>& b) -> decltype(L2sqrdist<nChannels>(a.data(),b.data())) {
        return L2sqrdist<nChannels>(a.data(),b.data());
    }

    /// computes the squared L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline auto L2sqrdist(const std::array<T,nChannels>& a, const T* const b) -> decltype(L2sqrdist<nChannels>(a.data(),b)) {
        return L2sqrdist<nChannels>(a.data(),b);
    }

    /// computes the squared L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline auto L2sqrdist(const T* const a, const std::array<T,nChannels>& b) -> decltype(L2sqrdist<nChannels>(a,b.data())) {
        return L2sqrdist<nChannels>(a,b.data());
    }

    /// computes the squared L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline auto L2sqrdist(const T* const a, const T* const b, size_t nElements, const uint8_t* m=NULL) -> decltype(L2sqrdist<nChannels>(a,b)) {
        decltype(L2sqrdist<nChannels>(a,b)) gResult = 0;
        size_t nTotElements = nElements*nChannels;
        if(m) {
            for(size_t n=0,i=0; n<nTotElements; n+=nChannels,++i)
                if(m[i])
                    gResult += L2sqrdist<nChannels>(a+n,b+n);
        }
        else {
            for(size_t n=0; n<nTotElements; n+=nChannels)
                gResult += L2sqrdist<nChannels>(a+n,b+n);
        }
        return gResult;
    }

    /// computes the squared L2 distance between two generic arrays
    template<typename T>
    inline auto L2sqrdist(const T* const a, const T* const b, size_t nElements, size_t nChannels, const uint8_t* m=NULL) -> decltype(L2sqrdist<3>(a,b,nElements,m)) {
        lvAssert_(nChannels>0 && nChannels<=4,"non-templated distance function only defined for 1 to 4 channels");
        switch(nChannels) {
            case 1: return L2sqrdist<1>(a,b,nElements,m);
            case 2: return L2sqrdist<2>(a,b,nElements,m);
            case 3: return L2sqrdist<3>(a,b,nElements,m);
            case 4: return L2sqrdist<4>(a,b,nElements,m);
            default: return 0;
        }
    }

#if USE_CVCORE_WITH_UTILS

    /// computes the squared L2 distance between two opencv vectors
    template<int nChannels, typename T>
    inline auto L2sqrdist(const cv::Vec<T,nChannels>& a, const cv::Vec<T,nChannels>& b) -> decltype(L2sqrdist<nChannels,T>(T(),T())) {
        T a_array[nChannels], b_array[nChannels];
        for(int c=0; c<nChannels; ++c) {
            a_array[c] = a[(int)c];
            b_array[c] = b[(int)c];
        }
        return L2sqrdist<nChannels>(a_array,b_array);
    }

#endif //USE_CVCORE_WITH_UTILS

    /// computes the L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline float L2dist(const T* const a, const T* const b) {
        static_assert(nChannels>0,"vectors should have at least one channel");
        decltype(L2sqrdist(*a,*b)) gResult = 0;
        for(size_t c=0; c<nChannels; ++c)
            gResult += L2sqrdist(a[c],b[c]);
        return sqrt((float)gResult);
    }

    /// computes the L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline auto L2dist(const std::array<T,nChannels>& a, const std::array<T,nChannels>& b) -> decltype(L2dist<nChannels>(a.data(),b.data())) {
        return L2dist<nChannels>(a.data(),b.data());
    }

    /// computes the L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline auto L2dist(const std::array<T,nChannels>& a, const T* const b) -> decltype(L2dist<nChannels>(a.data(),b)) {
        return L2dist<nChannels>(a.data(),b);
    }

    /// computes the L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline auto L2dist(const T* const a, const std::array<T,nChannels>& b) -> decltype(L2dist<nChannels>(a,b.data())) {
        return L2dist<nChannels>(a,b.data());
    }

    /// computes the L2 distance between two generic arrays
    template<size_t nChannels, typename T>
    inline float L2dist(const T* const a, const T* const b, size_t nElements, const uint8_t* m=NULL) {
        decltype(L2sqrdist<nChannels>(a,b)) gResult = 0;
        size_t nTotElements = nElements*nChannels;
        if(m) {
            for(size_t n=0,i=0; n<nTotElements; n+=nChannels,++i)
                if(m[i])
                    gResult += L2sqrdist<nChannels>(a+n,b+n);
        }
        else {
            for(size_t n=0; n<nTotElements; n+=nChannels)
                gResult += L2sqrdist<nChannels>(a+n,b+n);
        }
        return sqrt((float)gResult);
    }

    /// computes the squared L2 distance between two generic arrays
    template<typename T>
    inline float L2dist(const T* const a, const T* const b, size_t nElements, size_t nChannels, const uint8_t* m=NULL) {
        lvAssert_(nChannels>0 && nChannels<=4,"non-templated distance function only defined for 1 to 4 channels");
        switch(nChannels) {
            case 1: return L2dist<1>(a,b,nElements,m);
            case 2: return L2dist<2>(a,b,nElements,m);
            case 3: return L2dist<3>(a,b,nElements,m);
            case 4: return L2dist<4>(a,b,nElements,m);
            default: return 0;
        }
    }

#if USE_CVCORE_WITH_UTILS

    /// computes the L2 distance between two opencv vectors
    template<int nChannels, typename T>
    inline float L2dist(const cv::Vec<T,nChannels>& a, const cv::Vec<T,nChannels>& b) {
        T a_array[nChannels], b_array[nChannels];
        for(int c=0; c<nChannels; ++c) {
            a_array[c] = a[(int)c];
            b_array[c] = b[(int)c];
        }
        return L2dist<nChannels>(a_array,b_array);
    }

#endif //USE_CVCORE_WITH_UTILS

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// computes the color distortion between two integer arrays
    template<size_t nChannels, typename T>
    inline std::enable_if_t<std::is_integral<T>::value,size_t> cdist(const T* const curr, const T* const bg) {
        static_assert(nChannels>1,"vectors should have more than one channel");
        bool bNonConstDist = false;
        bool bNonNullDist = (curr[0]!=bg[0]);
        bool bNonNullBG = (bg[0]>0);
        for(size_t c=1; c<nChannels; ++c) {
            bNonConstDist |= (curr[c]!=curr[c-1]) || (bg[c]!=bg[c-1]);
            bNonNullDist |= (curr[c]!=bg[c]);
            bNonNullBG |= (bg[c]>0);
        }
        if(!bNonConstDist || !bNonNullDist)
            return 0;
        if(!bNonNullBG) {
            size_t nulldist = 0;
            for(size_t c=0; c<nChannels; ++c)
                nulldist += curr[c];
            return nulldist;
        }
        uint64_t curr_sqr = 0;
        uint64_t bg_sqr = 0;
        uint64_t mix = 0;
        for(size_t c=0; c<nChannels; ++c) {
            curr_sqr += uint64_t(curr[c]*curr[c]);
            bg_sqr += uint64_t(bg[c]*bg[c]);
            mix += uint64_t(curr[c]*bg[c]);
        }
        return (size_t)sqrt((float)(curr_sqr-(mix*mix)/bg_sqr));
    }

    /// computes the color distortion between two float arrays
    template<size_t nChannels, typename T>
    inline std::enable_if_t<std::is_floating_point<T>::value,T> cdist(const T* const curr, const T* const bg) {
        static_assert(nChannels>1,"vectors should have more than one channel");
        bool bNonConstDist = false;
        bool bNonNullDist = (curr[0]!=bg[0]);
        bool bNonNullBG = (bg[0]>0);
        for(size_t c=1; c<nChannels; ++c) {
            bNonConstDist |= (curr[c]!=curr[c-1]) || (bg[c]!=bg[c-1]);
            bNonNullDist |= (curr[c]!=bg[c]);
            bNonNullBG |= (bg[c]>0);
        }
        if(!bNonConstDist || !bNonNullDist)
            return 0;
        if(!bNonNullBG) {
            T nulldist = 0;
            for(size_t c=0; c<nChannels; ++c)
                nulldist += curr[c];
            return nulldist;
        }
        T curr_sqr = 0;
        T bg_sqr = 0;
        T mix = 0;
        for(size_t c=0; c<nChannels; ++c) {
            curr_sqr += curr[c]*curr[c];
            bg_sqr += bg[c]*bg[c];
            mix += curr[c]*bg[c];
        }
        if(curr_sqr<(mix*mix)/bg_sqr)
            return 0;
        else
            return sqrt(curr_sqr-(mix*mix)/bg_sqr);
    }

    /// computes the color distortion between two generic arrays
    template<size_t nChannels, typename T>
    inline auto cdist(const std::array<T,nChannels>& a, const std::array<T,nChannels>& b) -> decltype(cdist<nChannels>(a.data(),b.data())) {
        return cdist<nChannels>(a.data(),b.data());
    }

    /// computes the color distortion between two generic arrays
    template<size_t nChannels, typename T>
    inline auto cdist(const std::array<T,nChannels>& a, const T* const b) -> decltype(cdist<nChannels>(a.data(),b)) {
        return cdist<nChannels>(a.data(),b);
    }

    /// computes the color distortion between two generic arrays
    template<size_t nChannels, typename T>
    inline auto cdist(const T* const a, const std::array<T,nChannels>& b) -> decltype(cdist<nChannels>(a,b.data())) {
        return cdist<nChannels>(a,b.data());
    }

    /// computes the color distortion between two generic arrays
    template<size_t nChannels, typename T>
    inline auto cdist(const T* const a, const T* const b, size_t nElements, const uint8_t* m=NULL) -> decltype(cdist<nChannels>(a,b)) {
        decltype(cdist<nChannels>(a,b)) gResult = 0;
        size_t nTotElements = nElements*nChannels;
        if(m) {
            for(size_t n=0,i=0; n<nTotElements; n+=nChannels,++i)
                if(m[i])
                    gResult += cdist<nChannels>(a+n,b+n);
        }
        else {
            for(size_t n=0; n<nTotElements; n+=nChannels)
                gResult += cdist<nChannels>(a+n,b+n);
        }
        return gResult;
    }

    /// computes the color distortion between two generic arrays
    template<typename T>
    inline auto cdist(const T* const a, const T* const b, size_t nElements, size_t nChannels, const uint8_t* m=NULL) -> decltype(cdist<3>(a,b,nElements,m)) {
        lvAssert_(nChannels>1 && nChannels<=4,"non-templated distance function only defined for 2 to 4 channels");
        switch(nChannels) {
            case 2: return cdist<2>(a,b,nElements,m);
            case 3: return cdist<3>(a,b,nElements,m);
            case 4: return cdist<4>(a,b,nElements,m);
            default: return 0;
        }
    }

#if USE_CVCORE_WITH_UTILS

    /// computes the color distortion between two opencv vectors
    template<int nChannels, typename T>
    inline auto cdist(const cv::Vec<T,nChannels>& a, const cv::Vec<T,nChannels>& b) -> decltype(cdist<nChannels,T>(T(),T())) {
        T a_array[nChannels], b_array[nChannels];
        for(int c=0; c<nChannels; ++c) {
            a_array[c] = a[(int)c];
            b_array[c] = b[(int)c];
        }
        return cdist<nChannels>(a_array,b_array);
    }

#endif //USE_CVCORE_WITH_UTILS

    /// computes a color distortion-distance mix using two generic distances
    template<typename TL1Dist, typename TCDist>
    inline auto cmixdist(TL1Dist tL1Distance, TCDist tCDistortion) {
        return (tL1Distance/2+tCDistortion*4);
    }

    /// computes a color distortion-distance mix using two generic arrays
    template<size_t nChannels, typename T>
    inline auto cmixdist(const T* const curr, const T* const bg) {
        return cmixdist(L1dist<nChannels>(curr,bg),cdist<nChannels>(curr,bg));
    }

    /// computes a color distortion-distance mix using two generic arrays
    template<size_t nChannels, typename T>
    inline auto cmixdist(const std::array<T,nChannels>& a, const std::array<T,nChannels>& b) -> decltype(cmixdist<nChannels>(a.data(),b.data())) {
        return cmixdist<nChannels>(a.data(),b.data());
    }

    /// computes a color distortion-distance mix using two generic arrays
    template<size_t nChannels, typename T>
    inline auto cmixdist(const std::array<T,nChannels>& a, const T* const b) -> decltype(cmixdist<nChannels>(a.data(),b)) {
        return cmixdist<nChannels>(a.data(),b);
    }

    /// computes a color distortion-distance mix using two generic arrays
    template<size_t nChannels, typename T>
    inline auto cmixdist(const T* const a, const std::array<T,nChannels>& b) -> decltype(cmixdist<nChannels>(a,b.data())) {
        return cmixdist<nChannels>(a,b.data());
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// popcount LUT for 8-bit vectors
    static constexpr size_t s_anPopcntLUT8[256] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
    };

    /// computes the population count of an 8-bit vector using an 8-bit popcount LUT
    template<typename T>
    inline std::enable_if_t<(sizeof(T)==1),size_t> popcount(const T x) {
        static_assert(std::is_integral<T>::value,"type must be integral");
        return s_anPopcntLUT8[x];
    }

#if HAVE_POPCNT

    /// computes the population count of a 2- or 4-byte vector using 32-bit popcnt instruction
    template<typename T>
    inline std::enable_if_t<(sizeof(T)==2 || sizeof(T)==4),size_t> popcount(const T x) {
        static_assert(std::is_integral<T>::value,"type must be integral");
        return _mm_popcnt_u32((uint)x);
    }

    /// computes the population count of an 8-byte vector using 64-bit popcnt instruction
    template<typename T>
    inline std::enable_if_t<(sizeof(T)==8),size_t> popcount(const T x) {
        static_assert(std::is_integral<T>::value,"type must be integral");
        return _mm_popcnt_u64((uint64)x);
    }

#else //(!HAVE_POPCNT)

    /// computes the population count of an N-byte vector using an 8-bit popcount LUT
    template<typename T>
    inline std::enable_if_t<(sizeof(T)>1),size_t> popcount(const T x) {
        static_assert(std::is_integral<T>::value,"type must be integral");
        size_t nResult = 0;
        for(size_t l=0; l<sizeof(T); ++l)
            nResult += s_anPopcntLUT8[x>>l*8];
        return nResult;
    }

#endif //(!HAVE_POPCNT)

    /// computes the population count of a (nChannels*N)-byte vector
    template<size_t nChannels, typename T>
    inline size_t popcount(const T* x) {
        static_assert(nChannels>0,"vector should have at least one channel");
        size_t nResult = 0;
        for(size_t c=0; c<nChannels; ++c)
            nResult += popcount(x[c]);
        return nResult;
    }

    /// computes the population count of a (nChannels*N)-byte vector
    template<size_t nChannels, typename T>
    inline size_t popcount(const std::array<T,nChannels>& x) {
        return popcount<nChannels>(x.data());
    }

    /// computes the hamming distance between two N-byte vectors
    template<typename T>
    inline size_t hdist(T a, T b) {
        static_assert(std::is_integral<T>::value,"type must be integral");
        return popcount<T>(a^b);
    }

    /// computes the gradient magnitude distance between two N-byte vectors
    template<typename T>
    inline size_t gdist(T a, T b) {
        return L1dist(popcount(a),popcount(b));
    }

    /// computes the hamming distance between two (nChannels*N)-byte vectors
    template<size_t nChannels, typename T>
    inline size_t hdist(const T* const a, const T* const b) {
        static_assert(nChannels>0,"vectors should have at least one channel");
        T xor_array[nChannels];
        for(size_t c=0; c<nChannels; ++c)
            xor_array[c] = a[c]^b[c];
        return popcount<nChannels>(xor_array);
    }

    /// computes the hamming distance between two (nChannels*N)-byte vectors
    template<size_t nChannels, typename T>
    inline size_t hdist(const std::array<T,nChannels>& a, const std::array<T,nChannels>& b) {
        return hdist<nChannels>(a.data(),b.data());
    }

    /// computes the hamming distance between two (nChannels*N)-byte vectors
    template<size_t nChannels, typename T>
    inline size_t hdist(const std::array<T,nChannels>& a, const T* const b) {
        return hdist<nChannels>(a.data(),b);
    }

    /// computes the hamming distance between two (nChannels*N)-byte vectors
    template<size_t nChannels, typename T>
    inline size_t hdist(const T* const a, const std::array<T,nChannels>& b) {
        return hdist<nChannels>(a,b.data());
    }

    /// computes the gradient magnitude distance between two (nChannels*N)-byte vectors
    template<size_t nChannels, typename T>
    inline size_t gdist(const T* const a, const T* const b) {
        return L1dist(popcount<nChannels>(a),popcount<nChannels>(b));
    }

    /// computes the gradient magnitude distance between two (nChannels*N)-byte vectors
    template<size_t nChannels, typename T>
    inline size_t gdist(const std::array<T,nChannels>& a, const std::array<T,nChannels>& b) {
        return gdist<nChannels>(a.data(),b.data());
    }

    /// computes the gradient magnitude distance between two (nChannels*N)-byte vectors
    template<size_t nChannels, typename T>
    inline size_t gdist(const std::array<T,nChannels>& a, const T* const b) {
        return gdist<nChannels>(a.data(),b);
    }

    /// computes the gradient magnitude distance between two (nChannels*N)-byte vectors
    template<size_t nChannels, typename T>
    inline size_t gdist(const T* const a, const std::array<T,nChannels>& b) {
        return gdist<nChannels>(a,b.data());
    }

#if HAVE_GLSL

    inline std::string getShaderFunctionSource_absdiff(bool bUseBuiltinDistance) {
        // @@@@ test with/without built-in in final impl
        std::stringstream ssSrc;
        ssSrc << "uvec3 absdiff(in uvec3 a, in uvec3 b) {\n"
                 "    return uvec3(abs(ivec3(a)-ivec3(b)));\n"
                 "}\n"
                 "uint absdiff(in uint a, in uint b) {\n"
                 "    return uint(" << (bUseBuiltinDistance?"distance(a,b)":"abs((int)a-(int)b)") << ");\n"
                 "}\n";
        return ssSrc.str();
    }

    inline std::string getShaderFunctionSource_L1dist() {
        std::stringstream ssSrc;
        ssSrc << "uint L1dist(in uvec3 a, in uvec3 b) {\n"
                 "    ivec3 absdiffs = abs(ivec3(a)-ivec3(b));\n"
                 "    return uint(absdiffs.b+absdiffs.g+absdiffs.r);\n"
                 "}\n";
        return ssSrc.str();
    }

    inline std::string getShaderFunctionSource_L2dist(bool bUseBuiltinDistance) {
        std::stringstream ssSrc;
        ssSrc << "uint L2dist(in uvec3 a, in uvec3 b) {\n"
                 "    return uint(" << (bUseBuiltinDistance?"distance(a,b)":"sqrt(dot(ivec3(a)-ivec3(b)))") << ");\n"
                 "}\n";
        return ssSrc.str();
    }

    inline std::string getShaderFunctionSource_hdist() {
        std::stringstream ssSrc;
        ssSrc << "uvec3 hdist(in uvec3 a, in uvec3 b) {\n"
                 "    return bitCount(a^b);\n"
                 "}\n"
                 "uint hdist(in uint a, in uint b) {\n"
                 "    return bitCount(a^b);\n"
                 "}\n";
        return ssSrc.str();
    }

#endif //HAVE_GLSL

} // namespace lv