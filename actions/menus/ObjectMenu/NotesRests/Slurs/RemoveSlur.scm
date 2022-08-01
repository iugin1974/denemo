;;;RemoveSlur
;removes the slur that around the cursor position
(let ()
	(define (find-place move)
		(if (not (or (d-IsSlurStart)(d-IsSlurEnd)))
			(if (move)
						(find-place move))))
					
	(d-PushPosition)
	(if (d-IsSlurEnd)
		(begin
			(d-ToggleEndSlur))
			(d-PrevNote))
	(if (d-IsSlurStart)
		(d-NextNote))
	(find-place d-PrevNote)
	(if (d-IsSlurStart)
		(d-ToggleBeginSlur))
	(d-PopPosition)
	(d-PushPosition)
	(if (d-IsSlurStart)
		(begin
			(d-ToggleBeginSlur))
			(d-NextNote))
	(find-place d-NextNote)
	(if (d-IsSlurEnd)
		(d-ToggleEndSlur))
	(d-PopPosition))
