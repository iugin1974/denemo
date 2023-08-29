;;;;DynamicText
        ;; Standalone Dynamics - by Dan Wilckens. Tweaked by RTS. Note that for custom text the tag should be different.

(define-once DenemoDynamicList #f)

(let ((tag "DynamicText")(params DynamicText::params)(choice #f)(replace #f)(X 0) (level "63")(LilyString "")(CustomText #f)  (DynamicList "") )
  ; format: dynamics, midi volume, image filename
  (define DefaultDynamicList '(("fff" "127" "Fortississimo")("ff" "111" "Fortissimo") ("f" "95" "Forte") ("mf" "79" "MezzoForte")("mp" "63" "MezzoPiano") ("p" "47" "Piano") ("pp" "31" "Pianissimo") ("ppp" "15" "Pianississimo") ("More" "60" "") ("ppppp" "5" "ppppp")("pppp" "7" "pppp")("ffff" "127" "ffff") ("fp" "" "fp")
   ("sf" "" "sf") ("sff" "" "sff") ("sp" "" "sp" ) ("spp" "" "spp") ("sfz" "" "sfz") ("rfz" "" "rfz") ("Custom" "" "")))
  (define (firstmenu choices thelist)
      (if (and (not (null? thelist))
      (not (equal? (car (car thelist)) (_ "More"))))
    (begin
        (set! choices (string-append choices (car (car thelist)) stop))
        (firstmenu choices (cdr thelist)))
    (set! choice (d-GetOption (string-append choices (_ "More") stop)))))
   (if (equal? params "edit")
    (set! params #f))
  (if (d-Directive-standalone? tag)
    (set! replace #t))
   (d-PushPosition)
   (if (d-MoveCursorLeft)
        (if (d-Directive-standalone? tag)
            (begin
                (d-PopPosition)
                (d-MoveCursorLeft)
                (set! replace #t))
            (begin
                (d-PopPosition))))
            
(if params
    (begin
        (set! choice params)
        (set! DynamicList DefaultDynamicList))
    (begin
          (if DenemoDynamicList
              (set! DynamicList DenemoDynamicList)
              (set! DynamicList DefaultDynamicList))
          (firstmenu "" DynamicList)))
          
  (let loop ()
    (if (and choice (equal? choice (_ "More")) )
      (begin
    (if (not (equal? DenemoDynamicList DefaultDynamicList)) ;;we used a custom list, switch to using the Default list and try again
      (begin
        (set! DenemoDynamicList DefaultDynamicList)
        (set! DynamicList DefaultDynamicList)
        (firstmenu "" DynamicList)
        (loop))))))

 (if (and choice (equal? choice (_ "More")) )
      (begin        
    (set! DynamicList DefaultDynamicList)
    (set! choice (d-GetOption (string-append "ppppp" stop "pppp" stop  "ffff" stop "fp" stop "sf" stop  "sff" stop "sp"  stop "spp" stop "sfz" stop "rfz" stop "Custom" stop)))))


  (if choice
    (begin
		(set! LilyString (string-append " \\" choice  ))
		(set! X (assoc choice DynamicList) )
		(if X
			(begin
				(set! level (car (cdr X)) )
				(if (equal? choice "Custom" ) 
					(begin
						(set! LilyString
							(d-GetUserInputWithSnippets (_ "Text for Custom Dynamic")
								(_ "Give dynamic text to appear with note/chord:\nThe characters \\, \", ¶, { and } have a special meaning in the text,\nthe backslash \\ starts some LilyPond syntax, the others must be paired.\nRestrict use of the dynamic font to the letters mfprsz, see LilyPond documentation.")
							 "1x \\dynamic mp\n¶2x \\dynamic mf"))
						(if (pair? LilyString)
                            (let ((position (RadioBoxMenu (cons (_ "left align") "LEFT") (cons (_ "centered on note") "CENTER") (cons (_ "right align") "RIGHT"))))
                            (set! CustomText (car LilyString))
                            (set! LilyString (string-append "\\tweak DynamicText.self-alignment-X #" position " #(make-dynamic-script #{ \\markup \\normal-text \\column {" (cdr LilyString) " }#}) ")))
							(set! choice #f))
						(set! level "")))
				(if (equal? level "") 
				  (begin
						(if params
							(set! level "63")
							(set! level (d-GetUserInput (_ "Dynamic setting") (_ "Enter loudness level (0-127):") "63" )))
						(let ( (a 0)) 
								(set! a (string->number level) )    
								(if (or (boolean? a) (> a 127) (< a 0) )(set! level #f) )))))
			(begin ;;; how can this ever be executed???
				(set! level "127")
				(set! LilyString (string-append  " $(make-dynamic-script (markup #:normal-text #:bold #:italic \"" choice "\")) " ))))))
        
    (if choice
      (begin 
        (if (not replace) (d-DirectivePut-standalone tag))
			(d-DirectivePut-standalone-prefix tag  "<>")    
			(d-DirectivePut-standalone-minpixels tag  10)    
            (d-DirectivePut-standalone-postfix tag  LilyString)
            (if CustomText
				(d-DirectivePut-standalone-display  tag CustomText)
				(d-DirectivePut-standalone-graphic  tag (string-append "\n" choice "\nSerif\n24\n1\n1")))
            (d-DirectivePut-standalone-gy tag 40)
            (d-DirectivePut-standalone-gx tag 12)
            (if level
			   (begin
							(d-DirectivePut-standalone-override tag (logior DENEMO_OVERRIDE_STEP DENEMO_OVERRIDE_VOLUME))
							(d-DirectivePut-standalone-midibytes tag level)))
				(d-DirectivePut-standalone-minpixels tag 10)
				(d-SetSaved #f)
				(d-MoveCursorRight))))
(d-RefreshDisplay)
