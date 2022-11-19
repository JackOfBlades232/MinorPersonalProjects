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
procedure GiveUpEncounter(var enc: EncounterData; var ok: boolean);
procedure AttackerTryPlayCard(var enc: EncounterData; 
    c: CardPtr; var success: boolean);
procedure DefenderTryPlayCard(var enc: EncounterData;
    c: CardPtr; trump: CardSuit; var success: boolean);
function EncounterIsFull(var enc: EncounterData): boolean;


implementation

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

procedure InitEncounter(var enc: EncounterData;
    var attacker, defender: PlayerHandPtr);
begin
    enc.AttackerHand := attacker;
    enc.DefenderHand := defender;
    ResetTableCards(enc)
end;

procedure DisposeAndDeinitCard(var c: CardPtr);
begin
    dispose(c);
    c := nil
end;

procedure FinishEncounter(var enc: EncounterData);
var
    i: integer;
begin
    for i := 1 to NormalHandSize do
    begin
        if enc.AttackerTable[i] <> nil then
            DisposeAndDeinitCard(enc.AttackerTable[i]);
        if enc.DefenderTable[i] <> nil then
            DisposeAndDeinitCard(enc.DefenderTable[i])
    end
end;

procedure FlushTableToDefenderHand(var enc: EncounterData; 
    var t: TableCardArray; var success: boolean);
var
    i: integer;
    AddOk: boolean;
begin
    success := true;
    for i := 1 to NormalHandSize do
        if t[i] <> nil then
        begin
            AddCard(enc.DefenderHand^, t[i], AddOk);
            success := success and AddOk
        end
end;

procedure GiveUpEncounter(var enc: EncounterData; var ok: boolean);
begin
    ok := true;
    FlushTableToDefenderHand(enc, enc.AttackerTable, ok);
    FlushTableToDefenderHand(enc, enc.DefenderTable, ok);
    ResetTableCards(enc)
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

procedure TryPlayCard(var h: PlayerHandPtr; var t: TableCardArray;
    c: CardPtr; var success: boolean);
var
    i: integer;
begin
    RemoveCard(h^, c, success);
    if success then
        for i := 1 to NormalHandSize do
            if t[i] = nil then
            begin
                t[i] := c;
                break
            end
end;

function CardOfSameValueIsOnTable(var TableCards: TableCardArray;
    c: CardPtr): boolean;
var
    i: integer;
begin
    CardOfSameValueIsOnTable := false;
    for i := 1 to NormalHandSize do
    begin
        if (TableCards[i] <> nil) and (TableCards[i]^.value = c^.value) then
        begin
            CardOfSameValueIsOnTable := true;
            exit
        end
    end
end;

function AttackerCardCanBePlayed(var enc: EncounterData; c: CardPtr): boolean;
var
    CardsPlayed: integer;
begin
    CardsPlayed := NumCardsPlayed(enc.AttackerTable);

    AttackerCardCanBePlayed := (CardsPlayed < NormalHandSize) and (
        (CardsPlayed = 0) or
        CardOfSameValueIsOnTable(enc.AttackerTable, c) or
        CardOfSameValueIsOnTable(enc.DefenderTable, c)
    )
end;

procedure AttackerTryPlayCard(var enc: EncounterData; 
    c: CardPtr; var success: boolean);
begin
    success := AttackerCardCanBePlayed(enc, c);
    if success then
        TryPlayCard(enc.AttackerHand, enc.AttackerTable, c, success)
end;

procedure TryGetCardFromTable(var t: TableCardArray; 
    idx: integer; var out: CardPtr; var exists: boolean);
var
    i: integer;
begin
    exists := false;
    for i := 1 to NormalHandSize do
        if t[i] <> nil then
            if idx > 1 then
                idx := idx - 1
            else
            begin
                out := t[i];
                exists := true;
                break
            end
end;

function DefenderCardCanBePlayed(var enc: EncounterData;
    c: CardPtr; trump: CardSuit): boolean;
var
    CardGetOk: boolean;
    AttackerCardsPlayed, DefenderCardsPlayed: integer;
    AttackerCard: CardPtr;
begin
    AttackerCardsPlayed := NumCardsPlayed(enc.AttackerTable);
    DefenderCardsPlayed := NumCardsPlayed(enc.DefenderTable);
    DefenderCardCanBePlayed := (AttackerCardsPlayed - DefenderCardsPlayed) = 1;
    if not DefenderCardCanBePlayed then
        exit;

    TryGetCardFromTable(enc.AttackerTable, 
        AttackerCardsPlayed, AttackerCard, CardGetOk);
    DefenderCardCanBePlayed := 
        CardGetOk and (CompareCards(AttackerCard, c, trump) > 0)
end;

procedure DefenderTryPlayCard(var enc: EncounterData;
    c: CardPtr; trump: CardSuit; var success: boolean);
begin
    success := DefenderCardCanBePlayed(enc, c, trump);
    if success then
        TryPlayCard(enc.DefenderHand, enc.DefenderTable, c, success);
end;

function EncounterIsFull(var enc: EncounterData): boolean;
begin
    EncounterIsFull := (NumCardsPlayed(enc.AttackerTable) = NormalHandSize) and
                       (NumCardsPlayed(enc.DefenderTable) = NormalHandSize)
end;

end.
