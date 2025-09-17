// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MPTesting_CPlusPlus : ModuleRules
{
	public MPTesting_CPlusPlus(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput","OnlineSubsystemSteam","OnlineSubsystem","OnlineSubsystemUtils" });
	}
}
