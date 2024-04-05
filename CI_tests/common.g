WriteMatrixFile:=function(eFile, EXT)
    local output, eEXT, eVal;
    output:=OutputTextFile(eFile, true);
    AppendTo(output, Length(EXT), " ", Length(EXT[1]), "\n");
    for eEXT in EXT
    do
        for eVal in eEXT
        do
            AppendTo(output, " ", eVal);
        od;
        AppendTo(output, "\n");
    od;
    CloseStream(output);
end;

RemoveFileIfExist:=function(FileName)
    if IsExistingFile(FileName) then
        RemoveFile(FileName);
    fi;
end;

ListFileDirectory:=function(TheDir)
    local FileOUT, TheCommand, ListFiles, file, TheRead, TheReadRed;
    FileOUT:=Filename(DirectoryTemporary(), "LSfile");
    TheCommand:=Concatenation("ls ", TheDir, " > ", FileOUT);
    Exec(TheCommand);
    ListFiles:=[];
    file := InputTextFile(FileOUT);
    while(true)
    do
        TheRead:=ReadLine(file);
        if TheRead=fail then
            break;
        fi;
        len:=Length(TheRead);
        TheReadRed:=TheRead{[1..len-2]};
        Add(ListFiles, TheReadRed);
    od;
    CloseStream(file);
    RemoveFileIfExist(FileOUT);
    return ListFiles;
end;
