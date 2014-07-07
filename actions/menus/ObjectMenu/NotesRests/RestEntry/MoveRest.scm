;;;MoveRest
(if (d-Directive-chord? "PolyphonicRest")
    (begin
        (d-DirectiveDelete-chord "PolyphonicRest")
        (d-StagedDelete)))
(if (Rest?)
  (let* ((duration (string->number (d-GetNoteDuration)))(which (duration::lilypond->denemo duration )))
    (d-AddNoteToChord)
    (if (> duration 4)
        (begin
            (d-DirectivePut-standalone-minpixels "BeamingDisplayFix" 10)
            (d-MoveCursorRight)
            (d-DirectivePut-standalone-minpixels  "BeamingDisplayFix" 10)
            (d-MoveCursorLeft)
            (d-MoveCursorLeft)
            (d-RefreshDisplay)))
    (d-DirectivePut-chord-postfix "PolyphonicRest" "\\rest")
    (d-DirectivePut-chord-graphic  "PolyphonicRest" (vector-ref Rests which))
    (d-DirectivePut-chord-override "PolyphonicRest" (logior DENEMO_OVERRIDE_VOLUME DENEMO_OVERRIDE_GRAPHIC DENEMO_ALT_OVERRIDE))))
(d-SetSaved #f)
