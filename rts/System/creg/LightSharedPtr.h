#pragma once

#include "creg_cond.h"
#include "System/RefCnt.h"

#ifdef USING_CREG

namespace creg
{
	template<typename T>
	class LightSharedPtrType : public IType
	{
	public:
		LightSharedPtrType()
			: IType(sizeof(T))
			, ptrType(DeduceType <typename T::Type>::Get())
		{}
		~LightSharedPtrType() { }

		void Serialize(ISerializer* s, void* instance)
		{
			T& p = *(T*)instance;
			if (s->IsWriting()) {

			}
			//DeduceType<typename T::first_type>::Get()->Serialize(s,(void*) &p.first);
			//DeduceType<typename T::second_type>::Get()->Serialize(s,(void*) &p.second);
		}
		std::string GetName() const { return "LightSharedPtr<" + ptrType->GetName() + ">"; }
	private:
		std::unique_ptr<IType> ptrType;
	};

	template<typename T>
	class LightWeakPtrType : public IType
	{
	public:
		LightWeakPtrType() : IType(sizeof(T)) { }
		~LightWeakPtrType() { }

		void Serialize(ISerializer* s, void* instance)
		{
			T& p = *(T*)instance;
			//DeduceType<typename T::first_type>::Get()->Serialize(s,(void*) &p.first);
			//DeduceType<typename T::second_type>::Get()->Serialize(s,(void*) &p.second);
		}
		std::string GetName() const { return "LightWeakPtr<" + DeduceType<T>::Get()->GetName() + ">"; }
	};


	// LightSharedPtr type
	template<typename T>
	struct DeduceType<recoil::LightSharedPtr<T>> {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new LightSharedPtrType< recoil::LightSharedPtr<T> >());
		}
	};

	//
	template<typename T>
	struct DeduceType<recoil::LightWeakPtr<T>> {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new LightWeakPtrType< recoil::LightWeakPtr<T> >());
		}
	};
}

#endif // USING_CREG