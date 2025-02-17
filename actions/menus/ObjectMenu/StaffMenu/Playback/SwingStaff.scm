;;;SwingStaff
(let ((tag "Duration")(big 60)(small 40))
    (define (skip-tuplets)
        (if (Tupletmarker?)
            (while (and (d-NextObjectInMeasure) (not (Tupletmarker?))))
            (if (Tupletmarker?)
                (if (d-NextObjectInMeasure)
                    (skip-tuplets)))))
    (define (NextNoteInMeasure)
        (define start (d-GetHorizontalPosition))
        (skip-tuplets)
        (while (and (d-NextObjectInMeasure) (not (Tupletmarker?)) (not (Music?))))
        (if (Tupletmarker?)
            (skip-tuplets))
        (and (not (equal? start (d-GetHorizontalPosition))) (Music?)))

    (define (swing)
	   (if (positive? (d-GetDots))
			(begin
				(NextNoteInMeasure) 
				(NextNoteInMeasure)))
       (if (and (equal? (d-GetNoteDuration) "8") (not (d-IsGrace))
                (NextNoteInMeasure) 
                (equal? (d-GetNoteDuration) "8")  (not (d-IsGrace)))
        (begin 
            (d-MoveCursorLeft)
            (d-SetDurationInTicks big)
            (d-DirectivePut-chord-override tag (logior DENEMO_OVERRIDE_GRAPHIC DENEMO_ALT_OVERRIDE))
            (d-DirectivePut-chord-prefix tag "8")
            (d-MoveCursorRight)
            (d-SetDurationInTicks small)
            (d-DirectivePut-chord-override tag (logior DENEMO_OVERRIDE_GRAPHIC DENEMO_ALT_OVERRIDE))
            (d-DirectivePut-chord-prefix tag "8")
            )))
    (set! big (d-GetUserInput (_ "Swing Staff") (_ "Give percentage swing: ") "60"))
    (if (and big (string->number big) (< (string->number big) 99) (> (string->number big) 1))
        (begin
            (set! big (round (/ (* (string->number big) 384) 100)))
            (set! small (- 384 big))
            (d-MoveToBeginning)
            (skip-tuplets)
            (swing)
            (while (d-NextObject)
                (skip-tuplets)
                (swing))
            (d-CreateTimebase)      
            (d-SetSaved #f))))