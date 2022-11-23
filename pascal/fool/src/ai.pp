unit AI; { ai.pp }
interface
uses encounter, hand, deck, card;
{
    contains the functionalty for generating AI signals 
    (what card index to play)
    a dumb one, for that matter
}

procedure CollectAIInput(var CardIndex: integer; var enc: EncounterData;
    var h: PlayerHandPtr; var d: CardDeck; trump: CardSuit; var ok: boolean);

implementation
procedure CollectAIInput(var CardIndex: integer; var enc: EncounterData;
    var h: PlayerHandPtr; var d: CardDeck; trump: CardSuit; var ok: boolean);
var
    i: integer;
    c: CardPtr;
begin
    CardIndex := 0;
    ok := true;
    for i := 1 to NumCardsInHand(h^) do
    begin
        TryGetCardFromHand(h^, c, i, ok);
        if not ok then
            exit;
        if ((h = enc.AttackerHand) and AttackerCardCanBePlayed(enc, c)) or
           ((h = enc.DefenderHand) and DefenderCardCanBePlayed(enc, c, trump))
           then
        begin
            CardIndex := i;
            break
        end
    end;
end;

end.
