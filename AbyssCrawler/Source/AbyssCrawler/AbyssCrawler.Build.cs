// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AbyssCrawler : ModuleRules
{
	public AbyssCrawler(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
PublicDependencyModuleNames.AddRange(new string[] { 
	"Core", 
	"CoreUObject", 
	"Engine", 
	"InputCore", 
	"EnhancedInput", 
	"UMG", 
	"Slate", 
	"SlateCore", 
	"GameplayAbilities", 
    	"AIModule",         // AI 및 Behavior Tree용
    	"GameplayTasks",    // GAS 필수 모듈 1
    	"GameplayAbilities",// GAS 필수 모듈 2
    	"GameplayTags"      // GAS 필수 모듈 3 (FGameplayTag 사용을 위해)
});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		PublicIncludePaths.AddRange(new string[] {"AbyssCrawler"});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
