#include "SCTDTurretPart.h"

ESCTDTurretPartGrade USCTDTurretPart::GetGrade() const
{
	return GetGradeFromAdditionalOptionCount(GetAdditionalOptionCount());
}

int32 USCTDTurretPart::GetAdditionalOptionCount() const
{
	return AdditionalOptions.Num();
}

ESCTDTurretPartGrade USCTDTurretPart::GetGradeFromAdditionalOptionCount(int32 OptionCount)
{
	switch (FMath::Clamp(OptionCount, 0, 3))
	{
	case 0:
		return ESCTDTurretPartGrade::Common;
	case 1:
		return ESCTDTurretPartGrade::Advanced;
	case 2:
		return ESCTDTurretPartGrade::Rare;
	default:
		return ESCTDTurretPartGrade::Heroic;
	}
}
