#pragma once

#include "Commandlets/Commandlet.h"
#include "SolidDumpWorldChartCommandlet.generated.h"

/** Write /Game/World/WorldChart as JSON. tools\dump-chart.bat runs this. */
UCLASS()
class USolidDumpWorldChartCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USolidDumpWorldChartCommandlet();

	virtual int32 Main(const FString& Params) override;
};
