;;;;;StartBarre
(d-LilyPondDefinition (cons "stopBarre" "\\stopTextSpan"))
(d-LilyPondDefinition (cons "startBarre" "#(define-event-function (arg-string-qty str) 
  ((integer?) markup?)
  (let* (
         (mrkp (markup #:bold #:upright #:concat ( str #:hspace 0.3))))
  
    (define (width grob text-string)
      (let* ((layout (ly:grob-layout grob))
             (props (ly:grob-alist-chain 
                       grob 
                       (ly:output-def-lookup layout 'text-font-defaults))))
      (interval-length 
        (ly:stencil-extent 
          (interpret-markup layout props (markup text-string)) 
          X))))
    #{  
      \\tweak after-line-breaking 
        #(lambda (grob)
          (let* ((mrkp-width (width grob mrkp))
                 (line-thickness (ly:staff-symbol-line-thickness grob)))
           (ly:grob-set-nested-property! 
             grob 
             '(bound-details left padding) 
             (+ (/ mrkp-width -4) (* line-thickness 2)))))     
      \\tweak font-size -2
      \\tweak style #'line
      \\tweak bound-details.left.text #mrkp
      \\tweak bound-details.left.attach-dir -1
      \\tweak bound-details.left-broken.text ##f
      \\tweak bound-details.left-broken.attach-dir -1
      %% adjust the numeric values to fit your needs:
      \\tweak bound-details.left-broken.padding 1.5
      \\tweak bound-details.right-broken.padding 0
      \\tweak bound-details.right.padding 0.25
      \\tweak bound-details.right.attach-dir 2
      \\tweak bound-details.right-broken.text ##f
      \\tweak bound-details.right.text
        \\markup
          \\with-dimensions #'(0 . 0) #'(-.3 . 0) 
          \\draw-line #'(0 . -1)
      \\startTextSpan  
    #}))"))
(let* ((tag "StartBarre") (prefix (d-DirectiveGet-standalone-prefix tag)) (text (d-DirectiveGet-standalone-display tag)))
	(if text
		(d-DirectiveDelete-standalone tag)
		(begin
			(set! prefix "<>\\tweak extra-offset #'(0 . 0.7) ")
			(set! text "½CIII")))
	(set! text (d-GetUserInput (_ "Start Barre") (_ "Give text") text))
	(if text
		(begin
			(d-DirectivePut-standalone tag)
			(d-DirectivePut-standalone-display tag text)
			(d-DirectivePut-standalone-tx tag 20)
			(d-DirectivePut-standalone-minpixels tag 30)
			(d-DirectivePut-standalone-prefix tag prefix)
			(d-DirectivePut-standalone-postfix tag (string-append "\\startBarre \"" text "\" ")))))
(d-RefreshDisplay)	
