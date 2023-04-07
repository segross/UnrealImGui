#pragma once

struct FImguiContextHandle
{
public:
	FImguiContextHandle()
	{
		ContextName = NAME_None;
	}
	FImguiContextHandle(FName Name){ ContextName = Name;}
	FName GetContextName() const {  return ContextName; }
	bool IsValid(){ return ContextName != NAME_None;}
protected:
	FName ContextName;
};
