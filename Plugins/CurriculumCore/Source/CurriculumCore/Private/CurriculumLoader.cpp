#include "CurriculumLoader.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	bool ReadJson(const FString& Path, TSharedPtr<FJsonObject>& Out, TArray<FString>& Errors)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			Errors.Add(FString::Printf(TEXT("Bestand ontbreekt: %s"), *Path));
			return false;
		}

		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, Out) || !Out.IsValid())
		{
			Errors.Add(FString::Printf(TEXT("Ongeldige JSON in %s: %s"), *Path, *Reader->GetErrorMessage()));
			return false;
		}
		return true;
	}

	FString GetString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Value;
		Object->TryGetStringField(Field, Value);
		return Value;
	}

	int32 GetInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		double Value = 0.0;
		Object->TryGetNumberField(Field, Value);
		return static_cast<int32>(Value);
	}

	TArray<FString> GetStrings(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		TArray<FString> Result;
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (Object->TryGetArrayField(Field, Values))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				FString Text;
				if (Value.IsValid() && Value->TryGetString(Text))
				{
					Result.Add(Text);
				}
			}
		}
		return Result;
	}

	TArray<FName> GetNames(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		TArray<FName> Result;
		for (const FString& Text : GetStrings(Object, Field))
		{
			Result.Add(FName(*Text));
		}
		return Result;
	}

	FCurriculumStep ParseStep(const TSharedPtr<FJsonObject>& Object)
	{
		FCurriculumStep Step;
		Step.Id = GetString(Object, TEXT("id"));
		Step.Title = GetString(Object, TEXT("title"));
		Step.Instruction = GetString(Object, TEXT("instruction"));
		Step.Hints = GetStrings(Object, TEXT("hints"));
		Step.UiProfile = GetString(Object, TEXT("ui_profile"));

		const TSharedPtr<FJsonObject>* Highlight = nullptr;
		if (Object->TryGetObjectField(TEXT("highlight"), Highlight) && Highlight && Highlight->IsValid())
		{
			const FString Tab = GetString(*Highlight, TEXT("tab"));
			const FString Tag = GetString(*Highlight, TEXT("tag"));
			Step.Highlight.Tab = Tab.IsEmpty() ? NAME_None : FName(*Tab);
			Step.Highlight.Tag = Tag.IsEmpty() ? NAME_None : FName(*Tag);
		}
		return Step;
	}

	bool LoadChapter(const FString& Path, FCurriculumChapter& Out, TArray<FString>& Errors)
	{
		TSharedPtr<FJsonObject> Object;
		if (!ReadJson(Path, Object, Errors))
		{
			return false;
		}

		Out.Id = GetString(Object, TEXT("id"));
		Out.Number = GetInt(Object, TEXT("number"));
		Out.Title = GetString(Object, TEXT("title"));
		Out.Summary = GetString(Object, TEXT("summary"));
		Out.UiProfile = GetString(Object, TEXT("ui_profile"));

		const TArray<TSharedPtr<FJsonValue>>* Steps = nullptr;
		if (Object->TryGetArrayField(TEXT("steps"), Steps))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Steps)
			{
				const TSharedPtr<FJsonObject> StepObject = Value.IsValid() ? Value->AsObject() : nullptr;
				if (StepObject.IsValid())
				{
					Out.Steps.Add(ParseStep(StepObject));
				}
			}
		}

		if (Out.Id.IsEmpty() || Out.Steps.Num() == 0)
		{
			Errors.Add(FString::Printf(TEXT("Hoofdstuk zonder id of stappen: %s"), *Path));
			return false;
		}
		return true;
	}
}

FCurriculumData CurriculumLoader::Load(const FString& RootDir)
{
	FCurriculumData Data;

	TSharedPtr<FJsonObject> Index;
	if (!ReadJson(FPaths::Combine(RootDir, TEXT("curriculum.json")), Index, Data.Errors))
	{
		return Data;
	}

	for (const FString& Folder : GetStrings(Index, TEXT("tracks")))
	{
		const FString TrackDir = FPaths::Combine(RootDir, TEXT("tracks"), Folder);
		TSharedPtr<FJsonObject> TrackObject;
		if (!ReadJson(FPaths::Combine(TrackDir, TEXT("track.json")), TrackObject, Data.Errors))
		{
			continue;
		}

		FCurriculumTrack& Track = Data.Tracks.AddDefaulted_GetRef();
		Track.Id = GetString(TrackObject, TEXT("id"));
		Track.Number = GetInt(TrackObject, TEXT("number"));
		Track.Title = GetString(TrackObject, TEXT("title"));
		Track.Summary = GetString(TrackObject, TEXT("summary"));

		for (const FString& ChapterFile : GetStrings(TrackObject, TEXT("chapters")))
		{
			FCurriculumChapter Chapter;
			if (LoadChapter(FPaths::Combine(TrackDir, ChapterFile), Chapter, Data.Errors))
			{
				Track.Chapters.Add(MoveTemp(Chapter));
			}
		}
	}

	TArray<FString> ProfileFiles;
	const FString ProfileDir = FPaths::Combine(RootDir, TEXT("ui_profiles"));
	IFileManager::Get().FindFiles(ProfileFiles, *FPaths::Combine(ProfileDir, TEXT("*.json")), true, false);
	for (const FString& File : ProfileFiles)
	{
		TSharedPtr<FJsonObject> Object;
		if (!ReadJson(FPaths::Combine(ProfileDir, File), Object, Data.Errors))
		{
			continue;
		}
		FCurriculumUiProfile Profile;
		Profile.Id = GetString(Object, TEXT("id"));
		Profile.HideTabs = GetNames(Object, TEXT("hide_tabs"));
		Profile.ShowTabs = GetNames(Object, TEXT("show_tabs"));
		if (!Profile.Id.IsEmpty())
		{
			Data.UiProfiles.Add(Profile.Id, MoveTemp(Profile));
		}
	}

	return Data;
}
