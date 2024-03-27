#include "SolLuaInstancer.h"

#include "SolLuaDocument.h"
#include "SolLuaEventListener.h"


namespace Rml::SolLua
{

	ElementPtr SolLuaDocumentElementInstancer::InstanceElement(Element* parent, const String& tag, const XMLAttributes& attributes)
	{
		return ElementPtr(new SolLuaDocument(m_state, tag, m_lua_env_identifier));
	}

	void SolLuaDocumentElementInstancer::ReleaseElement(Element* element)
	{
		delete element;
	}


	EventListener* SolLuaEventListenerInstancer::InstanceEventListener(const String& value, Element* element)
	{
		return new SolLuaEventListener(m_state, value, element);
	}

} // end namespace Rml::SolLua
