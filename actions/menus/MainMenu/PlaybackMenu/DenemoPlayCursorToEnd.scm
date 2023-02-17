;DenemoPlayCurosrToEnd
(let ((timerId 0)(ontime   (d-GetMidiOnTime)))	
    (define (GetMeasureDuration)
		(let ((end #f)(start #f))
			(if (d-MoveToMeasureRight)
				(begin
					(set! end (d-GetTimeAtCursor))
					(d-MoveToMeasureLeft)
					(set! start (d-GetTimeAtCursor))
					(- end start)))))
	(d-SetPlaybackInterval ontime -1)
	(if (> (abs (- (d-AdjustPlaybackStart 0.0) ontime)) 0.0001)
		(begin ;without this Play starts before the playback interval is set!
			(disp "Not ready!")
			(d-AdjustPlaybackStart 0.0) ontime))
	(set! timerId (d-Timer  (inexact->exact (round (* 1000 (GetMeasureDuration)))) "(d-ScrollRight)"))			
	(d-Play (string-append "(d-KillTimer " (number->string timerId) ")")))
 
