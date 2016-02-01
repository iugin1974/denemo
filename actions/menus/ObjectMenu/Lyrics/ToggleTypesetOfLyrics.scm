;;ToggleTypesetLyrics
(d-PushPosition)
(let ((note #f))
  (define (do-staff)
     (d-TypesetLyricsForStaff (not   (d-TypesetLyricsForStaff)))) 
  (define (do-movement)
    (while (d-MoveToStaffUp))
    (do-staff)
    (while (d-MoveToStaffDown)
        (do-staff)))
  (while (d-PreviousMovement))
  (do-movement)
  (while (d-NextMovement)
        (do-movement)))
 (if (d-TypesetLyricsForStaff)
 	(d-InfoDialog (_ "Lyrics will appear in the Print View for the default layout"))
 	(d-InfoDialog (_ "Lyrics will be omitted  in the Print View for the default layout")))
(d-PopPosition)