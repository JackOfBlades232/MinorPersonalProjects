unit encounter; { encounter.pp }
{
    contains functionality for modelling one encounter of 2 hands
}
interface
uses hand, card;
type
    TableCardArray = array [1..NormalHandSize] of CardPtr;
    EncounterData = record
        AttackerHand, DefenderHand: PlayerHand;
        AttackerTable, DefenderTable: TableCardArray;
    end;

procedure InitEncounter(var enc: EncounterData,
    var attacker, defender: PlayerHand);

