unit encounter; { encounter.pp }
{
    contains functionality for modelling one encounter of 2 hands
}
interface
uses hand, card;
type
    TableCardArray = array [1..NormalHandSize] of CardPtr;
    EncounterData = record
        AttackerHand, DefenderHand: PlayerHandPtr;
        AttackerTable, DefenderTable: TableCardArray;
    end;

procedure InitEncounter(var enc: EncounterData;
    var attacker, defender: PlayerHandPtr);
procedure FinishEncounter(var enc: EncounterData);
procedure GiveUpEncounter(var enc: EncounterData);
procedure AttackerTryPlayCard(var enc: EncounterData; 
    c: CardPtr; var success: boolean);
procedure DefenderTryPlayCard(var enc: EncounterData;
    c: CardPtr; trump: CardSuit; var success: boolean);


implementation

procedure ResetTableCards(var enc: EncounterData); forward;

procedure InitEncounter(var enc: EncounterData;
    var attacker, defender: PlayerHandPtr);
begin
    enc.AttackerHand := attacker;
    enc.DefenderHand := defender;
    ResetTableCards(enc)
end;

procedure ResetTableCards(var enc: EncounterData);
var
    i: integer;
begin
    for i := 1 to NormalHandSize do
    begin
        enc.AttackerTable[i] := nil;
        enc.DefenderTable[i] := nil
    end
end;

procedure DisposeAndDeinitCard(var c: CardPtr);
var
    tmp: CardPtr;
begin
    tmp := c;
    c := nil;
    dispose(tmp)
end;

procedure FinishEncounter(var enc: EncounterData);
var
    i: integer;
    tmp: CardPtr;
begin
    for i := 1 to NormalHandSize do
    begin
        if enc.AttackerTable[i] <> nil then
            DisposeAndDeinitCard(enc.AttackerTable[i]);
        if enc.DefenderTable[i] <> nil then
            DisposeAndDeinitCard(enc.DefenderTable[i])
    end
end;

procedure GiveUpEncounter(var enc: EncounterData);
var
    i: integer;
    ok: boolean;
begin
    for i := 1 to NormalHandSize do
    begin
        if enc.AttackerTable[i] <> nil then
            AddCard(enc.DefenderHand^, enc.AttackerTable[i], ok);
        if enc.DefenderTable[i] <> nil then
            AddCard(enc.DefenderHand^, enc.DefenderTable[i], ok)
    end;
    ResetTableCards(enc)
end;

function NumCardsPlayed(var TableCards: TableCardArray): integer; forward;
procedure TryPlayCard(var h: PlayerHandPtr; var t: TableCardArray;
    c: CardPtr; var success: boolean); forward;

function AttackerCardCanBePlayed(var enc: EncounterData; c: CardPtr): boolean;
    forward;

procedure AttackerTryPlayCard(var enc: EncounterData; 
    c: CardPtr; var success: boolean);
begin
    success := AttackerCardCanBePlayed(enc, c);
    if success then
        TryPlayCard(enc.AttackerHand, enc.AttackerTable, c, success)
end;

function AttackerCardCanBePlayed(var enc: EncounterData; c: CardPtr): boolean;
var
    i, CardsPlayed: integer;
    TableCard: CardPtr;
begin
    CardsPlayed := NumCardsPlayed(enc.AttackerTable);
    AttackerCardCanBePlayed := CardsPlayed < NormalHandSize;
    if (CardsPlayed = 0) or (not AttackerCardCanBePlayed) then
        exit;

    AttackerCardCanBePlayed := false;
    for i := 1 to NormalHandSize do
    begin
        TableCard := enc.AttackerTable[i];
        if (TableCard <> nil) and (TableCard^.value = c^.value) then
        begin
            AttackerCardCanBePlayed := true;
            exit
        end
    end;
    for i := 1 to NormalHandSize do
    begin
        TableCard := enc.DefenderTable[i];
        if (TableCard <> nil) and (TableCard^.value = c^.value) then
        begin
            AttackerCardCanBePlayed := true;
            exit
        end
    end
end;

function DefenderCardCanBePlayed(var enc: EncounterData;
    c: CardPtr; trump: CardSuit): boolean; forward;

procedure DefenderTryPlayCard(var enc: EncounterData;
    c: CardPtr; trump: CardSuit; var success: boolean);
begin
    success := DefenderCardCanBePlayed(enc, c, trump);
    if success then
{$IFDEF DEBUG}
    begin
        writeln('Def card can be played');
{$ENDIF}
        TryPlayCard(enc.DefenderHand, enc.DefenderTable, c, success);
{$IFDEF DEBUG}
    end
{$ENDIF}
end;

function DefenderCardCanBePlayed(var enc: EncounterData;
    c: CardPtr; trump: CardSuit): boolean;
var
    AttackerCardsPlayed, DefenderCardsPlayed: integer;
    AttackerCard: CardPtr;
begin
    AttackerCardsPlayed := NumCardsPlayed(enc.AttackerTable);
    DefenderCardsPlayed := NumCardsPlayed(enc.DefenderTable);
    DefenderCardCanBePlayed := (AttackerCardsPlayed - DefenderCardsPlayed) = 1;
    if not DefenderCardCanBePlayed then
        exit;

    AttackerCard := enc.AttackerTable[AttackerCardsPlayed];
    DefenderCardCanBePlayed := CompareCards(AttackerCard^, c^, trump) > 0
end;

procedure TryPlayCard(var h: PlayerHandPtr; var t: TableCardArray;
    c: CardPtr; var success: boolean);
var
    i: integer;
begin
    RemoveCard(h^, c, success);
    if success then
{$IFDEF DEBUG}
    begin
        writeln('Def card was removed');
{$ENDIF}
        for i := 1 to NormalHandSize do
            if t[i] = nil then
            begin
                t[i] := c;
                break
            end
{$IFDEF DEBUG}
    end
{$ENDIF}
end;

function NumCardsPlayed(var TableCards: TableCardArray): integer;
var
    i: integer;
begin
    NumCardsPlayed := 0;
    for i := 1 to NormalHandSize do
        if TableCards[i] <> nil then
            NumCardsPlayed := NumCardsPlayed + 1
end;

end.
