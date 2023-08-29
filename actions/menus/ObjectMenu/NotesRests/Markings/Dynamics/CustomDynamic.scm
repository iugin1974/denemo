;;;CustomDynamic
(let ((text #f) (tag "CustomDynamic") (params CustomDynamic::params) (markup #f)(current #f)
            (position #f) (shift ""))
     (set! current (d-DirectiveGet-chord-data tag))
     (if (not current)
        (set! current (d-DirectiveGet-chord-display tag)))
     (if (and params (not (equal? "edit" params)))
        (begin
            (if (string? params)
                (set! text (cons params (string-append "\"" params "\"")))
                (let ((choice (caar params)))  ;;; params is a list of pairs
                     ;(set! text (cons current (string-append "\"" current "\""))) 
                        (case choice
                        ((padding)   (set! shift (string-append "-\\tweak #'padding #"  (cdar params) "  ")))
                        ((direction)  (set! position (cdar params)))            
                        ((offsetx)  (set! position (string-append "-\\tweak #'X-offset #'" (cdar params) " -\\tweak #'Y-offset #'"  (cdadr params) " -"))))))))
      (if (not position)
         (begin   
            (set! position (RadioBoxMenu (cons (_ "left align") "LEFT") (cons (_ "centered on note") "CENTER") (cons (_ "right align") "RIGHT")))))
            
     (if (not current)
                (set! current "")) 
    (if (not text)
        (set! text (d-GetUserInputWithSnippets (_ "Text for Custom Dynamic") (_ "Give dynamic text to appear with note/chord:\nThe characters \\, \", §, { and } have a special meaning in the text,\nthe backslash \\ starts some LilyPond syntax, the others must be paired.\nRestrict use of the dynamic font to the letters mfprsz, see LilyPond documentation.") current)));;cannot popup menu after this, it runs gtk_main
 
    (if text
        (if (string-null? (car text))
  
            (begin
                (let ((confirm (d-GetUserInput (d-DirectiveGet-chord-display tag) (_ "Delete this text?") (_ "y"))))
                 (if (equal? confirm (_ "y"))
                    (begin
                        (d-DirectiveDelete-chord tag)
                        (d-SetSaved #f))
                    (d-InfoDialog (_ "Cancelled")))))
            (begin 
                (if position
                   (begin
                            (set! markup (cdr text))
                            (set! text (car text))
                            (d-DirectivePut-chord-display tag (string-pad-right text 5))
                            (d-DirectivePut-chord-data tag text)
                            (d-DirectivePut-chord-postfix tag (string-append shift "\\tweak DynamicText.self-alignment-X #" position " #(make-dynamic-script #{ \\markup \\normal-text \\column {" markup " }#}) "))
                            (d-DirectivePut-chord-minpixels tag 20)
                            (d-SetSaved #f)))))))
            
