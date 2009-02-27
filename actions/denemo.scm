(use-modules (srfi srfi-13))
(use-modules (ice-9 regex))
(use-modules (ice-9 optargs))



(define Chord? (lambda ()
		  (string=? (d-GetType) "CHORD")))

(define NextChordInSelection (lambda () (if (d-NextSelectedObject) 
					    (if (Chord?)
			                	 #t
			                	 (NextChordInSelection))
					    #f
					    )))
(define FirstChordInSelection (lambda () (if (d-GoToMark)
						  (if (Chord?)
			                	 #t)
						  #f)))
				    
(define ApplyToSelection (lambda (command positioning_command)
			   (begin
			     (if (eval-string positioning_command)
				 (begin
				    (eval-string  command)
				    (ApplyToSelection command "(d-NextSelectedObject)"))))))
;;;;;;;;;;;;;;;;; ExtraOffset
(define* (ExtraOffset what  #:optional (type "chord") (context ""))
  (let ((oldstr #f) (start "") (end "") (get-command d-DirectiveGet-chord-prefix)  (put-command d-DirectivePut-chord-prefix))
    (cond
     ((string=? type "note")
      (begin (set! get-command d-DirectiveGet-note-prefix)
	     (set! put-command d-DirectivePut-note-prefix)))
     ((string=? type "standalone")
      (begin (set! get-command d-DirectiveGet-standalone-prefix)
	     (set! put-command d-DirectivePut-standalone-prefix)))
     )
    

    (set! oldstr (get-command what))
    (if (equal? oldstr "")
	(set! oldstr #f))
    (set! start (string-append "\\once \\override " context what "  #'extra-offset = #'("))
    (set! end ")")
    (put-command what (ChangeOffset oldstr start end))))

;;;;;;;;;;;;;;;;; ChangeOffset
;;; e.g.  (define prefixstring      "\\once \\override Fingering  #'extra-offset = #'(")
;;; (define postfix ")")
(define d-x "0.0")
(define d-y "0.0")

(define (ChangeOffset oldstr prefixstring postfixstring)
  (let ((startbit "")
	(endbit "")
	(theregexp "")
	(thematch "")
	(oldx "")
	(oldy "")
	(xold 0)
	(yold 0)
	(xnew "")
	(ynew "")
	(xval 0)
	(yval 0)
	(xy "")
	(offset ""))
    (begin
      (if (boolean? oldstr)
	  (set! oldstr (string-append prefixstring " 0.0 . 0.0 " postfixstring)))
      (set! startbit (regexp-quote prefixstring))
      (set! endbit  (regexp-quote postfixstring))
      (set! theregexp (string-append startbit "[ ]*([-0-9.]+)[ ]+.[ ]+([-0-9.]+)[ ]*" endbit))
      (set! thematch (string-match theregexp oldstr))
;;;get the old x y values out of oldstr
      (set! oldx (match:substring thematch 1))
      (set! oldy (match:substring thematch 2))

      (set! xold (string->number oldx))
      (set! yold (string->number oldy))
;;;add two values
      (set! offset (d-GetOffset))
      (if (pair? offset)
	  (begin
	    (set! d-x (car offset))
	    (set! d-y (cdr offset))
	    (set! xval (string->number d-x))
	    (set! yval (string->number d-y))
	    (set! xnew (number->string (+ xval xold)))
	    (set! ynew (number->string (+ yval yold)))
	    (set! xy (string-append xnew " . " ynew)))
	  (set! xy " 0.0 . 0.0 "))
      (regexp-substitute #f thematch 'pre (string-append prefixstring xy postfixstring) 'post))    
    ));;;; end of function change offset
;;;;;;;;;;;;;