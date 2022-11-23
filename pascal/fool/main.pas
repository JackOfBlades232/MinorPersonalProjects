program main; { main.pas }
{
    Main cycle for fool
}
uses crt, UI, AI, restock, encounter, deck, hand, card;
const
    MessageDuration = 1000;

procedure InitGame(
    var d: CardDeck; var trump: CardSuit; 
    var enc: EncounterData; var h1, h2: PlayerHand; 
    var player: PlayerHandPtr;
    var ok: boolean
);
begin
    randomize;
    GenerateDeck(d);
    TryGetTrumpSuit(d, trump, ok);
    if not ok then
        exit;

    InitHand(h1);
    InitHand(h2);
    InitRestockHands(h1, h2, d, ok);
    enc.AttackerHand := @h1;
    enc.DefenderHand := @h2;
    player := enc.AttackerHand
end;

procedure TryPlay(var enc: EncounterData; CardIndex: integer; trump: CardSuit;
    IsAttacker: boolean; var success: boolean);
var
    c: CardPtr;
begin
    success := false;
    if CardIndex = 0 then
    begin
        if IsAttacker then
        begin
            FinishEncounter(enc);
            success := true
        end
        else
            GiveUpEncounter(enc, success)
    end
    else if IsAttacker then
    begin
        TryGetCardFromHand(enc.AttackerHand^, c, CardIndex, success);
        if success then
            AttackerTryPlayCard(enc, c, success)
    end
    else
    begin
        TryGetCardFromHand(enc.DefenderHand^, c, CardIndex, success);
        if success then
            DefenderTryPlayCard(enc, c, trump, success)
    end
end;

procedure HandleTurn(var enc: EncounterData; var d: CardDeck; 
    trump: CardSuit; IsAttacker, IsPlayer: boolean);
var
    CardIndex: integer;
    completed: boolean = false;
    InputOk: boolean = false;
    CurrentHand: PlayerHandPtr;
begin
    if IsAttacker then
        CurrentHand := enc.AttackerHand
    else
        CurrentHand := enc.DefenderHand;
    while not completed do
    begin
        CardIndex := -1;
        if IsPlayer then
        begin
            DrawTurnInfo(d, trump, CurrentHand^, enc);
            CollectPlayerInput(CardIndex, InputOk)
        end
        else
            CollectAIInput(CardIndex, enc, CurrentHand, d, trump, InputOk); 
        if not InputOk then
            halt(0);
        if InputOk then
            TryPlay(enc, CardIndex, trump, IsAttacker, completed);
        if not completed then
        begin
            writeln('Can''t play this card!');
            delay(MessageDuration)
        end;
    end
end;

procedure SwapAttackerAndDefender(var enc: EncounterData);
var
    tmp: PlayerHandPtr;
begin
    tmp := enc.AttackerHand;
    enc.AttackerHand := enc.DefenderHand;
    enc.DefenderHand := tmp
end;

procedure HandleEncounter(var enc: EncounterData; 
    var d: CardDeck; var player: PlayerHandPtr; trump: CardSuit);
var
    IsAttackerTurn: boolean = true;
    IsPlayerTurn: boolean;
begin
    StartEncounter(enc);
    while enc.IsOngoing do
    begin
        IsPlayerTurn := (IsAttackerTurn and (player = enc.AttackerHand)) or
                        ((not IsAttackerTurn) and (player = enc.DefenderHand));
        HandleTurn(enc, d, trump, IsAttackerTurn, IsPlayerTurn);
        if IsAttackerTurn and (not enc.IsOngoing) then
            SwapAttackerAndDefender(enc);
        IsAttackerTurn := not IsAttackerTurn
    end;
    RestockHand(enc.AttackerHand^, d);
    RestockHand(enc.DefenderHand^, d)
end;

procedure DeinitGame(var d: CardDeck; var enc: EncounterData;
    var h1, h2: PlayerHand);
begin
    FinishEncounter(enc);
    DisposeOfHand(h1);
    DisposeOfHand(h2);
    DisposeOfDeck(d);
    clrscr
end;
        
var
    d: CardDeck;
    trump: CardSuit;
    enc: EncounterData;
    h1, h2: PlayerHand;
    player: PlayerHandPtr;

    ok: boolean;
begin
    InitGame(d, trump, enc, h1, h2, player, ok);
    if not ok then
    begin
        writeln(ErrOutput, 'Failed to initialize game!');
        halt(1)
    end;

    while (not HandIsEmpty(h1)) and (not HandIsEmpty(h2)) do
        HandleEncounter(enc, d, player, trump);

    DeinitGame(d, enc, h1, h2)
end.
