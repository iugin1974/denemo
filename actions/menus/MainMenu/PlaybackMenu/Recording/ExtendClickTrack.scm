;;;ExtendTheClickTrack
(let ((time 0)(rec-time (d-GetMidiRecordingDuration)))
	(if rec-time
		(begin
			(d-PushPosition)
			(while (d-MoveToStaffUp))
			(if (d-Directive-clef? DenemoClickTrack)
				(let ((num 0))
					(d-MoveToBeginning)
					
					(d-DirectiveDelete-standalone "MuteStaff");remove the speaker icon
					
					
					(d-EvenOutStaffLengths)
					(d-MoveToMeasureRight)
					(set! num (d-GetMeasuresInStaff))
					(if (> num 1)
						(d-DeleteFromCursorToEnd 'this))
					(set! time (d-GetTimeAtCursor))
					(d-EvenOutStaffLengths)
					(while (or (< time rec-time) (< (d-GetMeasure) num))
						(d-FillMeasure #t)
						
						(set! time (d-GetTimeAtCursor)))
					
					(DenemoSetPlaybackEnd)
					(d-MuteStaff)))
					(d-PopPosition))
		(d-WarningDialog (_ "No Midi recording"))))
