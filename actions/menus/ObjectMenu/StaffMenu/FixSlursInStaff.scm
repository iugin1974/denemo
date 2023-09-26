;;;FixSlursInStaff
(define-once CheckScore::ignore 0)
(define-once CheckScore::return #f)
(define-once CheckScore::error-position #f)
(d-PushPosition)
(d-MoveToBeginning)
(let ((params FixSlursInStaff::params)(start #f))
  (define (ToggleBeginSlur)
	(if params
		(begin
			(if (positive? CheckScore::ignore)
                    (set! CheckScore::ignore (1- CheckScore::ignore))
                    (begin
						(set! CheckScore::return (_ "Two Begin Slurs without End Slur"))
						(set! CheckScore::error-position start))))
		(d-ToggleBeginSlur)))
  (define (ToggleEndSlur)
	(if params
		(begin
			(if (positive? CheckScore::ignore)
						(set! CheckScore::ignore (1- CheckScore::ignore))
						(begin
				(set! CheckScore::return (_ "End Slur without Begin Slur"))
				(set! CheckScore::error-position (GetPosition)))))
		(d-ToggleEndSlur)))
  (let loop ()
    (if (d-IsSlurStart)
      (if start
		(ToggleBeginSlur)
		(set! start (GetPosition))))
	(if (d-IsSlurEnd)
	  (if start
		(begin
		  (set! start #f)
		  (if (d-IsSlurStart)
			(begin
			  (ToggleBeginSlur)
			  (ToggleEndSlur))))
		(ToggleEndSlur)))
	
    (if (d-NextChord)
      (loop)))
  (if start
    (begin
      (apply d-GoToPosition start)
      (ToggleBeginSlur))))
(d-PopPosition)
