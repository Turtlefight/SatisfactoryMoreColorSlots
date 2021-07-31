#pragma once

#include "Modules/ModuleManager.h"

#define MCS_BUILDABLE_COLORS_MAX_SLOTS 254

DECLARE_LOG_CATEGORY_EXTERN(LogMoreColorSlots, Log, Log);

class FMoreColorSlotsModule : public FDefaultGameModuleImpl {
public:
	virtual void StartupModule() override;

	virtual bool IsGameModule() const override { return true; }
};
