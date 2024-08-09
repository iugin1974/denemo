;;ToggleNoteUpTie
(let ((tag "Tie"))
    (if (Chord?)
        (begin
            (if (d-Directive-note? tag)
                (d-DirectiveDelete-note tag)
                (begin
					(if (d-IsTied) (d-ToggleTie))
                    (d-DirectivePut-note-postfix tag "^~ ")
                    (d-Chordize)
                    ;;for some reason we do not have ⏝ in the font
                    (d-DirectivePut-note-graphic tag "\n~up
                    Denemo
                    30")))
            (d-RefreshDisplay)
            (d-SetSaved #f))
        (d-WarningDialog (_ "Individual Ties only apply to notes within chords.. Do not put a tie on the chord as well."))))
