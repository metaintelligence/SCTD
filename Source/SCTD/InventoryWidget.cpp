#include "InventoryWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Model/Repository/SCTDDeckRepository.h"
#include "Model/Repository/SCTDPartsRepository.h"
#include "Model/Repository/SCTDUserRepository.h"
#include "SCTDMarqueeText.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"

namespace
{
	const FLinearColor InventoryChangedStatColor(0.32f, 0.42f, 1.0f, 1.0f);

	struct FInventoryPartBaseline
	{
		bool bValid = false;
		float BaseHealth = 0.0f;
		float Defense = 0.0f;
		float SelfRepairPerSecond = 0.0f;
		float MinAttackDamage = 0.0f;
		float MaxAttackDamage = 0.0f;
		float AttackSpeed = 0.0f;
		float AttackRange = 0.0f;
		float AreaAttackRange = 0.0f;
		float CriticalChance = 0.0f;
		float CriticalDamageMultiplier = 1.5f;
		int32 BuildCost = 0;
		float BuildTimeSeconds = 0.0f;
	};

	bool TryGetInventoryPartBaseline(const FSCTDOwnedTurretPartRecord& PartRecord, FInventoryPartBaseline& OutBaseline)
	{
		OutBaseline = FInventoryPartBaseline();
		if (PartRecord.DefinitionId == TEXT("part_body_pylon"))
		{
			OutBaseline.bValid = true;
			OutBaseline.BaseHealth = 400.0f;
			OutBaseline.Defense = 10.0f;
			OutBaseline.SelfRepairPerSecond = 1.0f;
			OutBaseline.BuildCost = 100;
			OutBaseline.BuildTimeSeconds = 5.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("part_body_quirass"))
		{
			OutBaseline.bValid = true;
			OutBaseline.BaseHealth = 600.0f;
			OutBaseline.Defense = 20.0f;
			OutBaseline.SelfRepairPerSecond = 2.0f;
			OutBaseline.BuildCost = 100;
			OutBaseline.BuildTimeSeconds = 2.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("part_body_gunstock"))
		{
			OutBaseline.bValid = true;
			OutBaseline.BaseHealth = 200.0f;
			OutBaseline.Defense = 10.0f;
			OutBaseline.SelfRepairPerSecond = 1.0f;
			OutBaseline.BuildCost = 100;
			OutBaseline.BuildTimeSeconds = 3.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("part_weapon_mortar"))
		{
			OutBaseline.bValid = true;
			OutBaseline.MinAttackDamage = 10.0f;
			OutBaseline.MaxAttackDamage = 20.0f;
			OutBaseline.AttackSpeed = 0.33f;
			OutBaseline.AttackRange = 5.0f;
			OutBaseline.AreaAttackRange = 1.0f;
			OutBaseline.CriticalChance = 0.10f;
			OutBaseline.CriticalDamageMultiplier = 1.50f;
			OutBaseline.BuildCost = 100;
			OutBaseline.BuildTimeSeconds = 5.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("part_weapon_minigun"))
		{
			OutBaseline.bValid = true;
			OutBaseline.MinAttackDamage = 4.0f;
			OutBaseline.MaxAttackDamage = 6.0f;
			OutBaseline.AttackSpeed = 3.0f;
			OutBaseline.AttackRange = 3.0f;
			OutBaseline.AreaAttackRange = 0.0f;
			OutBaseline.CriticalChance = 0.20f;
			OutBaseline.CriticalDamageMultiplier = 1.50f;
			OutBaseline.BuildCost = 100;
			OutBaseline.BuildTimeSeconds = 3.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("part_weapon_axe"))
		{
			OutBaseline.bValid = true;
			OutBaseline.MinAttackDamage = 9.0f;
			OutBaseline.MaxAttackDamage = 11.0f;
			OutBaseline.AttackSpeed = 1.0f;
			OutBaseline.AttackRange = 1.0f;
			OutBaseline.AreaAttackRange = 0.0f;
			OutBaseline.CriticalChance = 0.20f;
			OutBaseline.CriticalDamageMultiplier = 1.50f;
			OutBaseline.BuildCost = 100;
			OutBaseline.BuildTimeSeconds = 2.0f;
		}

		return OutBaseline.bValid;
	}

	bool IsChanged(float Left, float Right)
	{
		return !FMath::IsNearlyEqual(Left, Right, 0.01f);
	}

	bool IsChanged(int32 Left, int32 Right)
	{
		return Left != Right;
	}
}

void UInventoryWidget::SetUserRepository(USCTDUserRepository* NewUserRepository)
{
	UserRepository = NewUserRepository;
	RefreshAll();
}

TSharedRef<SWidget> UInventoryWidget::RebuildWidget()
{
	RefreshUsedPartLabels();
	RefreshEquipmentFilterOptions();
	RefreshAvailableFilterOptions();

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.006f, 0.007f, 0.009f, 1.0f))
		]
		+ SOverlay::Slot()
		.Padding(FMargin(30.0f, 24.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildHeader()
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
			[
				BuildFilterArea()
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 16.0f, 0.0f, 0.0f)
			[
				BuildGridArea()
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SAssignNew(HoverCardBox, SBox)
			.Visibility(EVisibility::Collapsed)
		];
}

void UInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!HoverCardBox || !HoveredPart.IsSet())
	{
		return;
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (PlayerController->GetMousePosition(MouseX, MouseY))
		{
			HoverCardBox->SetRenderTransform(FSlateRenderTransform(FVector2D(MouseX + 22.0f, MouseY + 18.0f)));
		}
	}
}

TSharedRef<SWidget> UInventoryWidget::BuildHeader()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(SSCTDMarqueeText)
			.Text(FText::FromString(TEXT("INVENTORY")))
			.ColorAndOpacity(FLinearColor(0.86f, 0.92f, 0.90f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
			.ContentPadding(FMargin(18.0f, 8.0f))
			.OnClicked(FOnClicked::CreateUObject(this, &UInventoryWidget::HandleBackClicked))
			[
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(TEXT("LOBBY")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildTabButton(const FString& Label, ESCTDTurretPartType PartType)
{
	return SNew(SButton)
		.ButtonColorAndOpacity(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda(
			[this, PartType]()
			{
				return FSlateColor(SelectedPartType == PartType
					? FLinearColor(0.18f, 0.22f, 0.86f, 1.0f)
					: FLinearColor(0.022f, 0.024f, 0.030f, 1.0f));
			})))
		.ContentPadding(FMargin(18.0f, 10.0f))
		.OnClicked(FOnClicked::CreateUObject(this, &UInventoryWidget::HandleTabClicked, PartType))
		[
			SNew(SSCTDMarqueeText)
			.Text(FText::FromString(Label))
			.ColorAndOpacity(FLinearColor(0.88f, 0.92f, 1.0f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.Justification(ETextJustify::Center)
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildFilterArea()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)[BuildTabButton(TEXT("BODY"), ESCTDTurretPartType::Base)]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f)[BuildTabButton(TEXT("WEAPON"), ESCTDTurretPartType::Weapon)]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[BuildTabButton(TEXT("CONTROL"), ESCTDTurretPartType::Control)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
		[
			BuildEquipmentFilterArea()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.018f, 0.020f, 0.026f, 0.96f))
			.Padding(10.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SSCTDMarqueeText)
					.Text(FText::FromString(TEXT("옵션 필터")))
					.ColorAndOpacity(FLinearColor(0.78f, 0.82f, 0.90f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.Justification(ETextJustify::Left)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ContentPadding(FMargin(10.0f, 5.0f))
					.OnClicked(FOnClicked::CreateUObject(this, &UInventoryWidget::HandleAddFilterClicked))
					[
					SNew(SSCTDMarqueeText)
					.Text(FText::FromString(TEXT("+")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
					.Justification(ETextJustify::Center)
				]
			]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SAssignNew(FilterBox, SScrollBox)
					.Orientation(Orient_Horizontal)
				]
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildEquipmentFilterArea()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(FLinearColor(0.018f, 0.020f, 0.026f, 0.96f))
		.Padding(10.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(TEXT("장비 필터")))
				.ColorAndOpacity(FLinearColor(0.78f, 0.82f, 0.90f, 1.0f))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.Justification(ETextJustify::Left)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				BuildItemFilterCombo()
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				BuildMountTypeFilterCombo()
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildGridArea()
{
	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarAlwaysVisible(true)
		+ SScrollBox::Slot()
		[
			SAssignNew(InventoryGrid, SUniformGridPanel)
			.SlotPadding(FMargin(4.0f))
		];

	RefreshFilterArea();
	RefreshGrid();
	return ScrollBox;
}

TSharedRef<SWidget> UInventoryWidget::BuildGridCell(const TOptional<FSCTDOwnedTurretPartRecord>& PartRecord)
{
	if (!PartRecord.IsSet())
	{
		return SNew(SBox)
			.WidthOverride(96.0f)
			.HeightOverride(72.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(0.014f, 0.015f, 0.018f, 1.0f))
			];
	}

	const FSCTDOwnedTurretPartRecord Item = PartRecord.GetValue();
	const bool bUsed = !GetUsedPartLabel(Item).IsEmpty();

	return SNew(SBox)
		.WidthOverride(96.0f)
		.HeightOverride(72.0f)
		[
			SNew(SButton)
			.ButtonColorAndOpacity(FLinearColor(0.018f, 0.020f, 0.026f, 1.0f))
			.ContentPadding(0.0f)
			.OnHovered(FSimpleDelegate::CreateUObject(this, &UInventoryWidget::HandleItemHovered, Item))
			.OnUnhovered(FSimpleDelegate::CreateUObject(this, &UInventoryWidget::HandleItemUnhovered))
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
					.BorderBackgroundColor(Item.RarityColor.CopyWithNewOpacity(0.38f))
					.Padding(2.0f)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
						.BorderBackgroundColor(FLinearColor(0.012f, 0.013f, 0.016f, 1.0f))
						.Padding(6.0f)
						[
							SNew(SSCTDMarqueeText)
							.Text(FText::FromString(Item.DisplayName))
							.ColorAndOpacity(Item.RarityColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							.TextLengthHandling(ESCTDTextLengthHandling::AnimateOnOverflow)
						]
					]
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.Padding(0.0f, 0.0f, 6.0f, 4.0f)
				[
					SNew(SSCTDMarqueeText)
					.Text(FText::FromString(bUsed ? TEXT("●") : TEXT("")))
					.ColorAndOpacity(FLinearColor(1.0f, 0.05f, 0.05f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				]
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildItemCard(const FSCTDOwnedTurretPartRecord& PartRecord) const
{
	const FString UsedLabel = GetUsedPartLabel(PartRecord);
	const FString Description = GetPartDescription(PartRecord);

	TSharedRef<SVerticalBox> OptionsBox = SNew(SVerticalBox);
	for (const FSCTDTurretPartOption& Option : PartRecord.AdditionalOptions)
	{
		OptionsBox->AddSlot().AutoHeight()
		[
			BuildStatLine(FString::Printf(TEXT("%s %s"), *GetOptionLabel(Option.OptionId), *BuildOptionValueText(Option)), FLinearColor(0.32f, 0.42f, 1.0f, 1.0f), 11)
		];
	}

	return SNew(SBox)
		.WidthOverride(360.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(PartRecord.RarityColor)
			.Padding(2.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(0.022f, 0.021f, 0.020f, 0.98f))
				.Padding(14.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(PartRecord.DisplayName))
						.ColorAndOpacity(PartRecord.RarityColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 10.0f)
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(FString::Printf(TEXT("%s / %s"), *GetPartTypeLabel(PartRecord.PartType), *GetMountTypeLabel(PartRecord.MountType))))
						.ColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.68f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(Description))
						.ColorAndOpacity(FLinearColor(0.52f, 0.52f, 0.54f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.Justification(ETextJustify::Center)
						.TextLengthHandling(ESCTDTextLengthHandling::RenderOverflow)
						.Visibility(Description.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(76.0f)
							.HeightOverride(112.0f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
								.BorderBackgroundColor(FLinearColor(0.045f, 0.045f, 0.048f, 1.0f))
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(14.0f, 0.0f, 0.0f, 0.0f)
						[
							BuildBasicStatsWidget(PartRecord)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						OptionsBox
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(UsedLabel))
						.ColorAndOpacity(FLinearColor(1.0f, 0.36f, 0.30f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					]
				]
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildStatLine(const FString& Text, const FLinearColor& Color, int32 FontSize) const
{
	return SNew(SSCTDMarqueeText)
		.Text(FText::FromString(Text))
		.ColorAndOpacity(Color)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", FontSize))
		.Justification(ETextJustify::Left);
}

TSharedRef<SWidget> UInventoryWidget::BuildBasicStatsWidget(const FSCTDOwnedTurretPartRecord& PartRecord) const
{
	FInventoryPartBaseline Baseline;
	const bool bHasBaseline = TryGetInventoryPartBaseline(PartRecord, Baseline);
	TSharedRef<SVerticalBox> StatsBox = SNew(SVerticalBox);

	auto AddRow = [&StatsBox, this](const FString& Label, const FString& BaseValue, const FString& CalculatedValue, bool bChanged)
	{
		StatsBox->AddSlot().AutoHeight()
		[
			BuildBasicStatRow(Label, BaseValue, CalculatedValue, bChanged)
		];
	};

	if (PartRecord.PartType == ESCTDTurretPartType::Base)
	{
		const float BaseHealth = bHasBaseline ? Baseline.BaseHealth : PartRecord.BaseHealth;
		const float BaseDefense = bHasBaseline ? Baseline.Defense : PartRecord.Defense;
		const float BaseRepair = bHasBaseline ? Baseline.SelfRepairPerSecond : PartRecord.SelfRepairPerSecond;
		const int32 BaseBuildCost = bHasBaseline ? Baseline.BuildCost : PartRecord.BuildCost;
		const float BaseBuildTime = bHasBaseline ? Baseline.BuildTimeSeconds : PartRecord.BuildTimeSeconds;
		AddRow(TEXT("Health"), FString::Printf(TEXT("%.0f"), BaseHealth), FString::Printf(TEXT("%.0f"), PartRecord.BaseHealth), bHasBaseline && IsChanged(BaseHealth, PartRecord.BaseHealth));
		AddRow(TEXT("Defense"), FString::Printf(TEXT("%.0f"), BaseDefense), FString::Printf(TEXT("%.0f"), PartRecord.Defense), bHasBaseline && IsChanged(BaseDefense, PartRecord.Defense));
		AddRow(TEXT("Repair"), FString::Printf(TEXT("%.1f / sec"), BaseRepair), FString::Printf(TEXT("%.1f / sec"), PartRecord.SelfRepairPerSecond), bHasBaseline && IsChanged(BaseRepair, PartRecord.SelfRepairPerSecond));
		AddRow(TEXT("Build Cost"), FString::Printf(TEXT("%d"), BaseBuildCost), FString::Printf(TEXT("%d"), PartRecord.BuildCost), bHasBaseline && IsChanged(BaseBuildCost, PartRecord.BuildCost));
		AddRow(TEXT("Build Time"), FString::Printf(TEXT("%.1fs"), BaseBuildTime), FString::Printf(TEXT("%.1fs"), PartRecord.BuildTimeSeconds), bHasBaseline && IsChanged(BaseBuildTime, PartRecord.BuildTimeSeconds));
	}
	else if (PartRecord.PartType == ESCTDTurretPartType::Weapon)
	{
		const float BaseMinDamage = bHasBaseline ? Baseline.MinAttackDamage : PartRecord.MinAttackDamage;
		const float BaseMaxDamage = bHasBaseline ? Baseline.MaxAttackDamage : PartRecord.MaxAttackDamage;
		const float BaseAttackSpeed = bHasBaseline ? Baseline.AttackSpeed : PartRecord.AttackSpeed;
		const float BaseAttackRange = bHasBaseline ? Baseline.AttackRange : PartRecord.AttackRange;
		const float BaseAreaRange = bHasBaseline ? Baseline.AreaAttackRange : PartRecord.AreaAttackRange;
		const float BaseCriticalChance = bHasBaseline ? Baseline.CriticalChance : PartRecord.CriticalChance;
		const float BaseCriticalDamage = bHasBaseline ? Baseline.CriticalDamageMultiplier : PartRecord.CriticalDamageMultiplier;
		const int32 BaseBuildCost = bHasBaseline ? Baseline.BuildCost : PartRecord.BuildCost;
		const float BaseBuildTime = bHasBaseline ? Baseline.BuildTimeSeconds : PartRecord.BuildTimeSeconds;
		AddRow(FString::Printf(TEXT("%s Damage"), *GetAttackAttributeLabel(PartRecord.AttackAttribute)), FString::Printf(TEXT("%.0f-%.0f"), BaseMinDamage, BaseMaxDamage), FString::Printf(TEXT("%.0f-%.0f"), PartRecord.MinAttackDamage, PartRecord.MaxAttackDamage), bHasBaseline && (IsChanged(BaseMinDamage, PartRecord.MinAttackDamage) || IsChanged(BaseMaxDamage, PartRecord.MaxAttackDamage)));
		AddRow(TEXT("Speed"), FString::Printf(TEXT("%.2f / sec"), BaseAttackSpeed), FString::Printf(TEXT("%.2f / sec"), PartRecord.AttackSpeed), bHasBaseline && IsChanged(BaseAttackSpeed, PartRecord.AttackSpeed));
		AddRow(TEXT("Range"), FString::Printf(TEXT("%.0f"), BaseAttackRange), FString::Printf(TEXT("%.0f"), PartRecord.AttackRange), bHasBaseline && IsChanged(BaseAttackRange, PartRecord.AttackRange));
		if (PartRecord.bCanAreaAttack)
		{
			AddRow(TEXT("Area"), FString::Printf(TEXT("%.0f"), BaseAreaRange), FString::Printf(TEXT("%.0f"), PartRecord.AreaAttackRange), bHasBaseline && IsChanged(BaseAreaRange, PartRecord.AreaAttackRange));
		}
		AddRow(TEXT("Crit"), FString::Printf(TEXT("%.0f%% / x%.2f"), BaseCriticalChance * 100.0f, BaseCriticalDamage), FString::Printf(TEXT("%.0f%% / x%.2f"), PartRecord.CriticalChance * 100.0f, PartRecord.CriticalDamageMultiplier), bHasBaseline && (IsChanged(BaseCriticalChance, PartRecord.CriticalChance) || IsChanged(BaseCriticalDamage, PartRecord.CriticalDamageMultiplier)));
		AddRow(TEXT("Build Cost"), FString::Printf(TEXT("%d"), BaseBuildCost), FString::Printf(TEXT("%d"), PartRecord.BuildCost), bHasBaseline && IsChanged(BaseBuildCost, PartRecord.BuildCost));
		AddRow(TEXT("Build Time"), FString::Printf(TEXT("%.1fs"), BaseBuildTime), FString::Printf(TEXT("%.1fs"), PartRecord.BuildTimeSeconds), bHasBaseline && IsChanged(BaseBuildTime, PartRecord.BuildTimeSeconds));
	}
	else
	{
		AddRow(TEXT("Targeting AI"), GetTargetingAILabel(PartRecord.TargetingAI), GetTargetingAILabel(PartRecord.TargetingAI), false);
	}

	return StatsBox;
}

TSharedRef<SWidget> UInventoryWidget::BuildBasicStatRow(const FString& Label, const FString& BaseValue, const FString& CalculatedValue, bool bChanged) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SSCTDMarqueeText)
			.Text(FText::FromString(FString::Printf(TEXT("%s: %s"), *Label, *BaseValue)))
			.ColorAndOpacity(FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
			.Justification(ETextJustify::Left)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SSCTDMarqueeText)
			.Text(FText::FromString(bChanged ? FString::Printf(TEXT("(%s)"), *CalculatedValue) : TEXT("")))
			.ColorAndOpacity(InventoryChangedStatColor)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
			.Justification(ETextJustify::Left)
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildItemFilterCombo()
{
	TSharedPtr<FName> InitiallySelected;
	for (const TSharedPtr<FName>& Item : AvailableItemFilterOptions)
	{
		if (Item.IsValid() && *Item == SelectedItemDefinitionFilterId)
		{
			InitiallySelected = Item;
			break;
		}
	}

	return SNew(SBox)
		.WidthOverride(180.0f)
		[
			SNew(SComboBox<TSharedPtr<FName>>)
			.OptionsSource(&AvailableItemFilterOptions)
			.InitiallySelectedItem(InitiallySelected)
			.OnSelectionChanged(SComboBox<TSharedPtr<FName>>::FOnSelectionChanged::CreateUObject(this, &UInventoryWidget::HandleItemFilterChanged))
			.OnGenerateWidget_Lambda([this](TSharedPtr<FName> Item)
			{
				return SNew(SSCTDMarqueeText)
					.Text(FText::FromString(Item.IsValid() ? GetItemFilterLabel(*Item) : TEXT("ALL ITEMS")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					.Justification(ETextJustify::Left);
			})
			[
				SNew(SSCTDMarqueeText)
				.Text_Lambda([this]()
				{
					return FText::FromString(GetItemFilterLabel(SelectedItemDefinitionFilterId));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildMountTypeFilterCombo()
{
	TSharedPtr<FName> InitiallySelected;
	for (const TSharedPtr<FName>& Item : AvailableMountTypeFilterOptions)
	{
		if (Item.IsValid() && *Item == SelectedMountTypeFilterId)
		{
			InitiallySelected = Item;
			break;
		}
	}

	return SNew(SBox)
		.WidthOverride(140.0f)
		.Visibility(SelectedPartType == ESCTDTurretPartType::Control ? EVisibility::Collapsed : EVisibility::Visible)
		[
			SNew(SComboBox<TSharedPtr<FName>>)
			.OptionsSource(&AvailableMountTypeFilterOptions)
			.InitiallySelectedItem(InitiallySelected)
			.OnSelectionChanged(SComboBox<TSharedPtr<FName>>::FOnSelectionChanged::CreateUObject(this, &UInventoryWidget::HandleMountTypeFilterChanged))
			.OnGenerateWidget_Lambda([this](TSharedPtr<FName> Item)
			{
				return SNew(SSCTDMarqueeText)
					.Text(FText::FromString(Item.IsValid() && !Item->IsNone() ? GetMountTypeLabel(GetMountTypeFromFilterId(*Item)) : TEXT("ALL TYPES")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					.Justification(ETextJustify::Left);
			})
			[
				SNew(SSCTDMarqueeText)
				.Text_Lambda([this]()
				{
					return FText::FromString(!SelectedMountTypeFilterId.IsNone() ? GetMountTypeLabel(GetMountTypeFromFilterId(SelectedMountTypeFilterId)) : TEXT("ALL TYPES"));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
		];
}

TSharedRef<SWidget> UInventoryWidget::BuildOptionCombo(int32 FilterIndex)
{
	TSharedPtr<FName> InitiallySelected;
	for (const TSharedPtr<FName>& Option : AvailableFilterOptions)
	{
		if (Option.IsValid() && SelectedFilterOptionIds.IsValidIndex(FilterIndex) && *Option == SelectedFilterOptionIds[FilterIndex])
		{
			InitiallySelected = Option;
			break;
		}
	}

	return SNew(SBox)
		.WidthOverride(180.0f)
		[
			SNew(SComboBox<TSharedPtr<FName>>)
			.OptionsSource(&AvailableFilterOptions)
			.InitiallySelectedItem(InitiallySelected)
			.OnSelectionChanged(SComboBox<TSharedPtr<FName>>::FOnSelectionChanged::CreateUObject(this, &UInventoryWidget::HandleFilterChanged, FilterIndex))
			.OnGenerateWidget_Lambda([this](TSharedPtr<FName> Item)
			{
				return SNew(SSCTDMarqueeText)
					.Text(FText::FromString(Item.IsValid() ? GetOptionLabel(*Item) : TEXT("-")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					.Justification(ETextJustify::Left);
			})
			[
				SNew(SSCTDMarqueeText)
				.Text_Lambda([this, FilterIndex]()
				{
					return FText::FromString(SelectedFilterOptionIds.IsValidIndex(FilterIndex) && !SelectedFilterOptionIds[FilterIndex].IsNone()
						? GetOptionLabel(SelectedFilterOptionIds[FilterIndex])
						: TEXT("Select option"));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
		];
}

FReply UInventoryWidget::HandleBackClicked()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/Lobby")));
	return FReply::Handled();
}

FReply UInventoryWidget::HandleTabClicked(ESCTDTurretPartType PartType)
{
	SelectedPartType = PartType;
	SelectedItemDefinitionFilterId = NAME_None;
	SelectedMountTypeFilterId = NAME_None;
	SelectedFilterOptionIds.Reset();
	RefreshAll();
	return FReply::Handled();
}

FReply UInventoryWidget::HandleAddFilterClicked()
{
	SelectedFilterOptionIds.Add(NAME_None);
	RefreshFilterArea();
	return FReply::Handled();
}

void UInventoryWidget::HandleItemFilterChanged(TSharedPtr<FName> NewValue, ESelectInfo::Type SelectInfo)
{
	SelectedItemDefinitionFilterId = NewValue.IsValid() ? *NewValue : NAME_None;
	RefreshGrid();
}

void UInventoryWidget::HandleMountTypeFilterChanged(TSharedPtr<FName> NewValue, ESelectInfo::Type SelectInfo)
{
	SelectedMountTypeFilterId = NewValue.IsValid() ? *NewValue : NAME_None;
	RefreshGrid();
}

void UInventoryWidget::HandleFilterChanged(TSharedPtr<FName> NewValue, ESelectInfo::Type SelectInfo, int32 FilterIndex)
{
	if (SelectedFilterOptionIds.IsValidIndex(FilterIndex))
	{
		SelectedFilterOptionIds[FilterIndex] = NewValue.IsValid() ? *NewValue : NAME_None;
		RefreshGrid();
	}
}

void UInventoryWidget::HandleItemHovered(FSCTDOwnedTurretPartRecord PartRecord)
{
	HoveredPart = PartRecord;
	if (HoverCardBox)
	{
		HoverCardBox->SetContent(BuildItemCard(PartRecord));
		HoverCardBox->SetVisibility(EVisibility::HitTestInvisible);
	}
}

void UInventoryWidget::HandleItemUnhovered()
{
	HoveredPart.Reset();
	if (HoverCardBox)
	{
		HoverCardBox->SetVisibility(EVisibility::Collapsed);
	}
}

void UInventoryWidget::RefreshAll()
{
	RefreshUsedPartLabels();
	RefreshEquipmentFilterOptions();
	RefreshAvailableFilterOptions();
	RefreshFilterArea();
	RefreshGrid();
}

void UInventoryWidget::RefreshEquipmentFilterOptions()
{
	AvailableItemFilterOptions.Reset();
	AvailableMountTypeFilterOptions.Reset();
	AvailableItemFilterOptions.Add(MakeShared<FName>(NAME_None));
	AvailableMountTypeFilterOptions.Add(MakeShared<FName>(NAME_None));
	AvailableMountTypeFilterOptions.Add(MakeShared<FName>(TEXT("TOWER")));
	AvailableMountTypeFilterOptions.Add(MakeShared<FName>(TEXT("CANNON")));
	AvailableMountTypeFilterOptions.Add(MakeShared<FName>(TEXT("ARM")));

	TSet<FName> SeenDefinitionIds;
	const USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	if (!PartsRepository)
	{
		return;
	}

	for (const FSCTDOwnedTurretPartRecord& PartRecord : PartsRepository->GetOwnedPartsByType(SelectedPartType))
	{
		if (!PartRecord.DefinitionId.IsNone() && !SeenDefinitionIds.Contains(PartRecord.DefinitionId))
		{
			SeenDefinitionIds.Add(PartRecord.DefinitionId);
			AvailableItemFilterOptions.Add(MakeShared<FName>(PartRecord.DefinitionId));
		}
	}
}

void UInventoryWidget::RefreshAvailableFilterOptions()
{
	AvailableFilterOptions.Reset();
	TSet<FName> SeenOptions;

	auto AddFilterOption = [&SeenOptions, this](FName OptionId)
	{
		if (!OptionId.IsNone() && !SeenOptions.Contains(OptionId))
		{
			SeenOptions.Add(OptionId);
			AvailableFilterOptions.Add(MakeShared<FName>(OptionId));
		}
	};

	if (SelectedPartType == ESCTDTurretPartType::Base)
	{
		AddFilterOption(TEXT("IncreaseHealth"));
		AddFilterOption(TEXT("IncreaseDefense"));
		AddFilterOption(TEXT("IncreaseSelfRepair"));
		AddFilterOption(TEXT("IncreaseAttackSpeed"));
		AddFilterOption(TEXT("IncreaseCriticalChance"));
		AddFilterOption(TEXT("IncreaseCriticalDamage"));
	}
	else if (SelectedPartType == ESCTDTurretPartType::Weapon)
	{
		AddFilterOption(TEXT("IncreaseAttackRange"));
		AddFilterOption(TEXT("PhysicalDamageBonus"));
		AddFilterOption(TEXT("FireDamageBonus"));
		AddFilterOption(TEXT("LightningDamageBonus"));
		AddFilterOption(TEXT("FrostDamageBonus"));
		AddFilterOption(TEXT("IncreaseAttackSpeed"));
		AddFilterOption(TEXT("IncreaseAreaRange"));
		AddFilterOption(TEXT("IncreaseCriticalChance"));
		AddFilterOption(TEXT("IncreaseCriticalDamage"));
	}
	else
	{
		AddFilterOption(TEXT("PhysicalDamageBonus"));
		AddFilterOption(TEXT("FireDamageBonus"));
		AddFilterOption(TEXT("LightningDamageBonus"));
		AddFilterOption(TEXT("FrostDamageBonus"));
		AddFilterOption(TEXT("IncreaseHealth"));
		AddFilterOption(TEXT("IncreaseDefense"));
		AddFilterOption(TEXT("IncreaseSelfRepair"));
		AddFilterOption(TEXT("IncreaseCriticalChance"));
		AddFilterOption(TEXT("IncreaseCriticalDamage"));
		AddFilterOption(TEXT("AmplifyStatusChance"));
		AddFilterOption(TEXT("PhysicalDestruction"));
		AddFilterOption(TEXT("PhysicalConcussion"));
		AddFilterOption(TEXT("FireIgnite"));
		AddFilterOption(TEXT("FireTileBurn"));
		AddFilterOption(TEXT("LightningStagger"));
		AddFilterOption(TEXT("LightningExecute"));
		AddFilterOption(TEXT("FrostChill"));
		AddFilterOption(TEXT("FrostFreeze"));
	}

	AddFilterOption(TEXT("ReduceBuildCost"));
	AddFilterOption(TEXT("ReduceBuildTime"));
	AddFilterOption(TEXT("ExtraScrapGain"));

	const USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	if (!PartsRepository)
	{
		return;
	}

	for (const FSCTDOwnedTurretPartRecord& PartRecord : PartsRepository->GetOwnedPartsByType(SelectedPartType))
	{
		for (const FSCTDTurretPartOption& Option : PartRecord.AdditionalOptions)
		{
			AddFilterOption(Option.OptionId);
		}
	}
}

void UInventoryWidget::RefreshFilterArea()
{
	if (!FilterBox)
	{
		return;
	}

	FilterBox->ClearChildren();
	for (int32 Index = 0; Index < SelectedFilterOptionIds.Num(); ++Index)
	{
		FilterBox->AddSlot().Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			BuildOptionCombo(Index)
		];
	}
}

void UInventoryWidget::RefreshGrid()
{
	if (!InventoryGrid)
	{
		return;
	}

	InventoryGrid->ClearChildren();
	const TArray<FSCTDOwnedTurretPartRecord> Parts = GetFilteredParts();
	for (int32 Index = 0; Index < MaxInventoryItemCount; ++Index)
	{
		TOptional<FSCTDOwnedTurretPartRecord> PartRecord;
		if (Parts.IsValidIndex(Index))
		{
			PartRecord = Parts[Index];
		}

		InventoryGrid->AddSlot(Index % GridColumnCount, Index / GridColumnCount)
		[
			BuildGridCell(PartRecord)
		];
	}
}

void UInventoryWidget::RefreshUsedPartLabels()
{
	UsedPartLabels.Reset();
	const USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (!DeckRepository)
	{
		return;
	}

	const TArray<FSCTDTurretDeckRecord> Decks = DeckRepository->GetDecks();
	for (int32 DeckIndex = 0; DeckIndex < Decks.Num(); ++DeckIndex)
	{
		for (const FSCTDPreparedTurretRecord& Turret : Decks[DeckIndex].Turrets)
		{
			const FString UsedLabel = FString::Printf(TEXT("사용 중 (Deck %d, %s)"), DeckIndex + 1, *Turret.DisplayName);
			UsedPartLabels.Add(Turret.BasePartInstanceId, UsedLabel);
			UsedPartLabels.Add(Turret.WeaponPartInstanceId, UsedLabel);
			UsedPartLabels.Add(Turret.ControlPartInstanceId, UsedLabel);
		}
	}
}

TArray<FSCTDOwnedTurretPartRecord> UInventoryWidget::GetFilteredParts() const
{
	const USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	if (!PartsRepository)
	{
		return {};
	}

	TArray<FSCTDOwnedTurretPartRecord> Result;
	for (const FSCTDOwnedTurretPartRecord& PartRecord : PartsRepository->GetOwnedPartsByType(SelectedPartType))
	{
		if (DoesPartPassFilters(PartRecord))
		{
			Result.Add(PartRecord);
		}
	}
	return Result;
}

bool UInventoryWidget::DoesPartPassFilters(const FSCTDOwnedTurretPartRecord& PartRecord) const
{
	if (!SelectedItemDefinitionFilterId.IsNone() && PartRecord.DefinitionId != SelectedItemDefinitionFilterId)
	{
		return false;
	}

	if (PartRecord.PartType != ESCTDTurretPartType::Control
		&& !SelectedMountTypeFilterId.IsNone()
		&& GetMountTypeFilterId(PartRecord.MountType) != SelectedMountTypeFilterId)
	{
		return false;
	}

	for (const FName FilterOptionId : SelectedFilterOptionIds)
	{
		if (FilterOptionId.IsNone())
		{
			continue;
		}

		const bool bHasOption = PartRecord.AdditionalOptions.ContainsByPredicate([FilterOptionId](const FSCTDTurretPartOption& Option)
		{
			return Option.OptionId == FilterOptionId;
		});
		if (!bHasOption)
		{
			return false;
		}
	}
	return true;
}

FString UInventoryWidget::GetPartTypeLabel(ESCTDTurretPartType PartType) const
{
	switch (PartType)
	{
	case ESCTDTurretPartType::Base:
		return TEXT("BODY");
	case ESCTDTurretPartType::Weapon:
		return TEXT("WEAPON");
	case ESCTDTurretPartType::Control:
		return TEXT("CONTROL");
	default:
		return TEXT("-");
	}
}

FString UInventoryWidget::GetMountTypeLabel(ESCTDTurretMountType MountType) const
{
	switch (MountType)
	{
	case ESCTDTurretMountType::Tower:
		return TEXT("TOWER");
	case ESCTDTurretMountType::Cannon:
		return TEXT("CANNON");
	case ESCTDTurretMountType::Arm:
		return TEXT("ARM");
	default:
		return TEXT("-");
	}
}

FString UInventoryWidget::GetAttackAttributeLabel(ESCTDAttackAttribute AttackAttribute) const
{
	switch (AttackAttribute)
	{
	case ESCTDAttackAttribute::Physical:
		return TEXT("Physical");
	case ESCTDAttackAttribute::Fire:
		return TEXT("Fire");
	case ESCTDAttackAttribute::Lightning:
		return TEXT("Lightning");
	case ESCTDAttackAttribute::Frost:
		return TEXT("Frost");
	default:
		return TEXT("-");
	}
}

FName UInventoryWidget::GetMountTypeFilterId(ESCTDTurretMountType MountType) const
{
	switch (MountType)
	{
	case ESCTDTurretMountType::Tower:
		return TEXT("TOWER");
	case ESCTDTurretMountType::Cannon:
		return TEXT("CANNON");
	case ESCTDTurretMountType::Arm:
		return TEXT("ARM");
	default:
		return NAME_None;
	}
}

ESCTDTurretMountType UInventoryWidget::GetMountTypeFromFilterId(FName MountTypeFilterId) const
{
	if (MountTypeFilterId == TEXT("CANNON"))
	{
		return ESCTDTurretMountType::Cannon;
	}
	if (MountTypeFilterId == TEXT("ARM"))
	{
		return ESCTDTurretMountType::Arm;
	}
	return ESCTDTurretMountType::Tower;
}

FString UInventoryWidget::GetTargetingAILabel(ESCTDTargetingAI TargetingAI) const
{
	switch (TargetingAI)
	{
	case ESCTDTargetingAI::Closer: return TEXT("CLOSER");
	case ESCTDTargetingAI::Sniper: return TEXT("SNIPER");
	case ESCTDTargetingAI::Greedy: return TEXT("GREEDY");
	case ESCTDTargetingAI::Potato: return TEXT("POTATO");
	case ESCTDTargetingAI::Chaser: return TEXT("CHASER");
	case ESCTDTargetingAI::Revenge: return TEXT("REVENGE");
	default: return TEXT("-");
	}
}

FString UInventoryWidget::GetPartDescription(const FSCTDOwnedTurretPartRecord& PartRecord) const
{
	if (!PartRecord.Description.IsEmpty())
	{
		return PartRecord.Description;
	}

	if (PartRecord.DefinitionId == TEXT("part_body_pylon"))
	{
		return TEXT("저가형 파일런은 타워형 무기를 탑재할 수 있다.");
	}
	if (PartRecord.DefinitionId == TEXT("part_body_quirass"))
	{
		return TEXT("저가형 갑옷으로 양팔형 무기를 탑재할 수 있다.");
	}
	if (PartRecord.DefinitionId == TEXT("part_body_gunstock"))
	{
		return TEXT("저가형 개머리판으로 캐논형 무기를 탑재할 수 있다.");
	}
	if (PartRecord.DefinitionId == TEXT("part_weapon_mortar"))
	{
		return TEXT("느리지만 장거리 광역 공격이 가능한 박격포이다.");
	}
	if (PartRecord.DefinitionId == TEXT("part_weapon_minigun"))
	{
		return TEXT("중거리 대응 사격에 탁월한 미니건이다.");
	}
	if (PartRecord.DefinitionId == TEXT("part_weapon_axe"))
	{
		return TEXT("강력한 근거리 공격으로 무엇도 놓치지 않는 도끼이다.");
	}
	if (PartRecord.DefinitionId == TEXT("part_control_closer"))
	{
		return TEXT("가장 가까운 몬스터를 공격하는 AI");
	}
	if (PartRecord.DefinitionId == TEXT("part_control_sniper"))
	{
		return TEXT("가장 먼 몬스터를 공격하는 AI");
	}
	if (PartRecord.DefinitionId == TEXT("part_control_greedy"))
	{
		return TEXT("가장 체력이 적은 몬스터를 공격하는 AI");
	}
	if (PartRecord.DefinitionId == TEXT("part_control_potato"))
	{
		return TEXT("가장 체력이 많은 몬스터를 공격하는 AI");
	}
	if (PartRecord.DefinitionId == TEXT("part_control_chaser"))
	{
		return TEXT("가장 빠른 몬스터를 공격하는 AI");
	}
	if (PartRecord.DefinitionId == TEXT("part_control_revenge"))
	{
		return TEXT("가장 공격력이 강한 몬스터를 공격하는 AI");
	}
	return TEXT("");
}

FString UInventoryWidget::GetItemFilterLabel(FName DefinitionId) const
{
	if (DefinitionId.IsNone())
	{
		return TEXT("ALL ITEMS");
	}

	const USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	if (PartsRepository)
	{
		for (const FSCTDOwnedTurretPartRecord& PartRecord : PartsRepository->GetOwnedPartsByType(SelectedPartType))
		{
			if (PartRecord.DefinitionId == DefinitionId)
			{
				return PartRecord.DisplayName;
			}
		}
	}

	return DefinitionId.ToString();
}

FString UInventoryWidget::GetOptionLabel(FName OptionId) const
{
	return OptionId.IsNone() ? TEXT("-") : OptionId.ToString();
}

FString UInventoryWidget::BuildOptionValueText(const FSCTDTurretPartOption& Option) const
{
	if (FMath::Abs(Option.Value) <= 1.0f)
	{
		return FString::Printf(TEXT("+%.0f%%"), Option.Value * 100.0f);
	}
	return FString::Printf(TEXT("+%.1f"), Option.Value);
}

FString UInventoryWidget::GetUsedPartLabel(const FSCTDOwnedTurretPartRecord& PartRecord) const
{
	if (const FString* UsedLabel = UsedPartLabels.Find(PartRecord.InstanceId))
	{
		return *UsedLabel;
	}
	return TEXT("");
}
