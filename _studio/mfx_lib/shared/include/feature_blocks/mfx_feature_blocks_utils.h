// Copyright (c) 2019-2021 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "mfx_common.h"

#include "mfxvideo.h"
#include <memory>
#include <map>
#include <list>
#include <exception>
#include <functional>
#include <algorithm>
#include <thread>
#include <assert.h>

namespace MfxFeatureBlocks
{
#if defined(MFX_ENABLE_FEATURE_BLOCKS_TRACE)

#define DEFINE_ERR_CODE(code)\
    {code, #code}

static const std::map<mfxStatus, const char*> StringsOfStatus =
{
    DEFINE_ERR_CODE(MFX_ERR_NONE                    ),
    DEFINE_ERR_CODE(MFX_ERR_UNKNOWN                 ),
    DEFINE_ERR_CODE(MFX_ERR_NULL_PTR                ),
    DEFINE_ERR_CODE(MFX_ERR_UNSUPPORTED             ),
    DEFINE_ERR_CODE(MFX_ERR_MEMORY_ALLOC            ),
    DEFINE_ERR_CODE(MFX_ERR_NOT_ENOUGH_BUFFER       ),
    DEFINE_ERR_CODE(MFX_ERR_INVALID_HANDLE          ),
    DEFINE_ERR_CODE(MFX_ERR_LOCK_MEMORY             ),
    DEFINE_ERR_CODE(MFX_ERR_NOT_INITIALIZED         ),
    DEFINE_ERR_CODE(MFX_ERR_NOT_FOUND               ),
    DEFINE_ERR_CODE(MFX_ERR_MORE_DATA               ),
    DEFINE_ERR_CODE(MFX_ERR_MORE_SURFACE            ),
    DEFINE_ERR_CODE(MFX_ERR_ABORTED                 ),
    DEFINE_ERR_CODE(MFX_ERR_DEVICE_LOST             ),
    DEFINE_ERR_CODE(MFX_ERR_INCOMPATIBLE_VIDEO_PARAM),
    DEFINE_ERR_CODE(MFX_ERR_INVALID_VIDEO_PARAM     ),
    DEFINE_ERR_CODE(MFX_ERR_UNDEFINED_BEHAVIOR      ),
    DEFINE_ERR_CODE(MFX_ERR_DEVICE_FAILED           ),
    DEFINE_ERR_CODE(MFX_ERR_MORE_BITSTREAM          ),
    DEFINE_ERR_CODE(MFX_ERR_GPU_HANG                ),
    DEFINE_ERR_CODE(MFX_ERR_REALLOC_SURFACE         ),
    DEFINE_ERR_CODE(MFX_ERR_RESOURCE_MAPPED         ),
    DEFINE_ERR_CODE(MFX_ERR_NOT_IMPLEMENTED         ),

    DEFINE_ERR_CODE(MFX_WRN_IN_EXECUTION            ),
    DEFINE_ERR_CODE(MFX_WRN_DEVICE_BUSY             ),
    DEFINE_ERR_CODE(MFX_WRN_VIDEO_PARAM_CHANGED     ),
    DEFINE_ERR_CODE(MFX_WRN_PARTIAL_ACCELERATION    ),
    DEFINE_ERR_CODE(MFX_WRN_INCOMPATIBLE_VIDEO_PARAM),
    DEFINE_ERR_CODE(MFX_WRN_VALUE_NOT_CHANGED       ),
    DEFINE_ERR_CODE(MFX_WRN_OUT_OF_RANGE            ),
    DEFINE_ERR_CODE(MFX_WRN_FILTER_SKIPPED          ),
    DEFINE_ERR_CODE(MFX_ERR_NONE_PARTIAL_OUTPUT     ),
    DEFINE_ERR_CODE(MFX_WRN_ALLOC_TIMEOUT_EXPIRED   ),
    DEFINE_ERR_CODE(MFX_TASK_WORKING                ),
    DEFINE_ERR_CODE(MFX_TASK_BUSY                   ),
    DEFINE_ERR_CODE(MFX_ERR_MORE_DATA_SUBMIT_TASK   )
};

inline const std::string GetMFXStatusString(mfxStatus mfxSts)
{
    auto it = StringsOfStatus.find(mfxSts);
    if (it != StringsOfStatus.end())
    {
        return it->second;
    }
    
    return std::to_string(mfxSts);
}
#endif // MFX_ENABLE_FEATURE_BLOCKS_TRACE

struct Storable
{
    virtual ~Storable() {}
};

template<class T>
class StorableRef
    : public Storable
    , public std::reference_wrapper<T>
{
public:
    StorableRef(T& ref)
        : std::reference_wrapper<T>(ref)
    {}
};

template<class T>
class MakeStorable
    : public T
    , public StorableRef<T>
{
public:
    template<class... TArg>
    MakeStorable(TArg&& ...arg)
        : T(std::forward<TArg>(arg)...)
        , StorableRef<T>((T&)*this)
    {}
};

template<class T>
class MakeStorablePtr
    : public std::unique_ptr<T>
    , public StorableRef<T>
{
public:
    using std::unique_ptr<T>::get;

    MakeStorablePtr(T* p)
        : std::unique_ptr<T>(p)
        , StorableRef<T>((T&)*get())
    {}
};

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T, typename... Args>
std::unique_ptr<typename std::conditional<std::is_base_of<Storable, T>::value, T, MakeStorable<T>>::type>
    make_storable(Args&&... args)
{
    return make_unique<typename std::conditional<std::is_base_of<Storable, T>::value, T, MakeStorable<T>>::type>
        (std::forward<Args>(args)...);
}

class StorageR
{
public:
    typedef mfxU32 TKey;
    static const TKey KEY_INVALID = TKey(-1);

    template<class T>
    const T& Read(TKey key) const
    {
        auto it = m_map.find(key);
        if (it == m_map.end())
            throw std::logic_error("Requested object was not found in storage");
        return dynamic_cast<T&>(*it->second);
    }

    bool Contains(TKey key) const
    {
        return (m_map.find(key) != m_map.end());
    }

    bool Empty() const
    {
        return m_map.empty();
    }

protected:
    std::map<TKey, std::unique_ptr<Storable>> m_map;
};

class StorageW : public StorageR
{
public:
    template<class T>
    T& Write(TKey key) const
    {
        auto it = m_map.find(key);
        if (it == m_map.end())
            throw std::logic_error("Requested object was not found in storage");
        return dynamic_cast<T&>(*it->second);
    }
};

class StorageRW : public StorageW
{
public:
    bool TryInsert(TKey key, std::unique_ptr<Storable>&& pObj)
    {
        return m_map.emplace(key, std::move(pObj)).second;
    }

    void Insert(TKey key, std::unique_ptr<Storable>&& pObj)
    {
        if (!TryInsert(key, std::move(pObj)))
            throw std::logic_error("Keys must be unique");
    }

    template<class T, typename std::enable_if<!std::is_base_of<Storable, T>::value, int>::type = 0>
    void Insert(TKey key, T* pObj)
    {
        Insert(key, new MakeStorablePtr<T>(pObj));
    }

    template<class T, typename = typename std::enable_if<std::is_base_of<Storable, T>::value>::type>
    void Insert(TKey key, T* pObj)
    {
        Insert(key, std::unique_ptr<T>(pObj));
    }

    void Erase(TKey key)
    {
        m_map.erase(key);
    }

    void Clear()
    {
        m_map.clear();
    }
};

template<StorageR::TKey K, class T>
struct StorageVar
{
    static const StorageR::TKey Key = K;
    typedef typename std::conditional<std::is_base_of<Storable, T>::value, T, StorableRef<T>>::type TStore;
    typedef T TRef;

    static const TRef& Get(const StorageR& s)
    {
        return s.Read<TStore>(Key);
    }

    static TRef& Get(StorageW& s)
    {
        return s.Write<TStore>(Key);
    }

    template<typename... Args>
    static TRef& GetOrConstruct(StorageRW& s, Args&&... args)
    {
        if (!s.Contains(Key))
            s.Insert(Key, make_storable<TRef>(std::forward<Args>(args)...));
        return s.Write<TStore>(Key);
    }
};

inline mfxStatus GetWorstSts(mfxStatus sts1, mfxStatus sts2)
{
    mfxStatus sts_min = std::min<mfxStatus>(sts1, sts2);
    return sts_min == MFX_ERR_NONE ? std::max<mfxStatus>(sts1, sts2) : sts_min;
}

template<class T>
inline void ThrowIf(bool bThrow, T sts)
{
    if (bThrow)
        throw sts;
}

template<class STS, class T, class... Args>
inline STS Catch(STS dflt, T func, Args&&... args)
{
    try
    {
        func(std::forward<Args>(args)...);
    }
    catch (STS sts)
    {
        return sts;
    }

    return dflt;
}

inline bool IgnoreSts(mfxStatus) { return false; }

template<
    class TBlock
    , typename std::enable_if<!std::is_same<typename std::remove_reference<TBlock>::type::TCall::result_type, mfxStatus>::value, int>::type = 0
    , class... TArgs>
inline mfxStatus CallAndGetMfxSts(TBlock&& blk, TArgs&&... args)
{
#if defined(MFX_ENABLE_FEATURE_BLOCKS_TRACE)
    typename std::remove_reference<TBlock>::type::TTracer tr(blk, blk.m_featureName, blk.m_blockName);
#endif // MFX_ENABLE_FEATURE_BLOCKS_TRACE

    blk.Call(std::forward<TArgs>(args)...);
    return MFX_ERR_NONE;
}

template<
    class TBlock
    , typename = typename std::enable_if<std::is_same<typename std::remove_reference<TBlock>::type::TCall::result_type, mfxStatus>::value>::type
    , class... TArgs>
inline mfxStatus CallAndGetMfxSts(TBlock&& blk, TArgs&&... args)
{
#if defined(MFX_ENABLE_FEATURE_BLOCKS_TRACE)
    typename std::remove_reference<TBlock>::type::TTracer tr(blk, blk.m_featureName, blk.m_blockName);
#endif // MFX_ENABLE_FEATURE_BLOCKS_TRACE

    mfxStatus sts = blk.Call(std::forward<TArgs>(args)...);

#if defined(MFX_ENABLE_FEATURE_BLOCKS_TRACE)
    if (sts != MFX_ERR_NONE)
    {
        std::stringstream threadID;
        threadID << std::this_thread::get_id();
        std::string stsString = GetMFXStatusString(sts);
        printf("TH#%s %s::%s -> %d::%d: Return %s\n"
            , threadID.str().c_str(), blk.m_featureName, blk.m_blockName
            , blk.FeatureID, blk.BlockID, stsString.c_str());
        fflush(stdout);
    }
#endif // MFX_ENABLE_FEATURE_BLOCKS_TRACE

    return sts;
}

template <class TPred, class TQ, class... TArgs>
mfxStatus RunBlocks(TPred stopAtSts, TQ& queue, TArgs&&... args)
{
    mfxStatus wrn = MFX_ERR_NONE, sts;
    using TBlock = typename TQ::value_type;
    auto RunBlock = [&](const TBlock& blk)
    {
        auto sts = CallAndGetMfxSts(blk, std::forward<TArgs>(args)...);
        ThrowIf(stopAtSts(sts), sts);
        wrn = GetWorstSts(sts, wrn);
    };

    try
    {
        sts = Catch(
            MFX_ERR_NONE
            , std::for_each<decltype(queue.begin()), decltype(RunBlock)>
            , queue.begin(), queue.end(), RunBlock
        );
    }
    catch (const std::exception & ex)
    {
        sts = MFX_ERR_UNKNOWN;
#if defined(_DEBUG)
        printf("EHW Exception: %s\n", ex.what());
        fflush(stdout);
#else
        std::ignore = ex;
#endif
    }

    return GetWorstSts(sts, wrn);
}

template<class TRV, class... TArg>
struct CallChain
    : public std::function<TRV(TArg...)>
{
    typedef std::function<TRV(TArg...)> TExt;
    typedef std::function<TRV(TExt, TArg...)> TInt;

    void Push(TInt newCall)
    {
        m_prev.push_front(*this);
        auto pPrev = &m_prev.front();
        (TExt&)*this = TExt([=](TArg... args)
        {
            return newCall(*pPrev, args...);
        });
    }

    std::list<TExt> m_prev;
};

} //namespace MfxFeatureBlocks

