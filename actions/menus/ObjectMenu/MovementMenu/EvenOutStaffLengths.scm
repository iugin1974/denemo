;;EvenOutStaffLengths
(define-once CheckScore::ignore 0)
(define-once CheckScore::return #f)
(define-once CheckScore::error-position #f)

(d-PushPosition)
(d-GoToPosition #f 1 1 1)
(let ((maxmeasures (d-GetMeasuresInStaff)) (uneven #f)(params EvenOutStaffLengths::params))
	(let loop ()
		(define measures (d-GetMeasuresInStaff))
		(if (not (= maxmeasures measures))
			(begin
			(set! uneven measures)
			(if (> measures maxmeasures)
			  (set! maxmeasures measures))))
		(if (d-MoveToStaffDown)
			(loop)))
	(d-GoToPosition #f 1 1 1)
	(if uneven
		(if params
		  (begin
			  (if (positive? CheckScore::ignore)
						(set! CheckScore::ignore (1- CheckScore::ignore))
						(begin
							(set! CheckScore::error-position (GetPosition))
							(set! CheckScore::return (_ "Staffs have different numbers of measures")))))
		  (begin
			(let loop ()
			  (define needed (- maxmeasures (d-GetMeasuresInStaff)))	      
			  (let addloop ((num needed))
			  (if (positive? num)
				  (begin
				(d-MoveToEnd)
				(d-SetSaved #f)
				(d-InsertMeasureAfter)
				(addloop (- num 1)))))
			  (if (d-MoveToStaffDown)
				(loop)))))))
(d-PopPosition)
;;;;;;
