;;;;BeginOrEndTriplet
;Begin or End Triplet
;Inserts a start triplet object or a stop tuplet if one is already started. Notes inside these objects have 2/3 or their written duration.
;triplets extending over barlines and nested triplets will be typeset correctly, but the display will not show them properly.
(let ((position (GetPosition)))
	(if (Appending?)
		(d-MoveCursorLeft))
	(let loop ()
				(if (TupletOpen?)
					(begin
						(apply d-GoToPosition position)
						(d-EndTuplet))
					(if (TupletClose?)
						(begin
							(apply d-GoToPosition position)
							(d-StartTriplet))				
						(if (d-PrevObjectInMeasure)
							(loop)
							(begin
								(apply d-GoToPosition position)
								(d-StartTriplet)))))))