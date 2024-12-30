;TextSpanStart
(if (Music?)
    (let ((tag "TextSpanStart")(text #f)(params TextSpanStart::params) (direction ""))
        (if (equal? params "edit")
            (set! params (RadioBoxMenu (cons (_ "Edit") 'edit)  (cons (_ "Delete") 'delete) (cons (_ "Advanced") 'advanced))))
            
        (set! text (d-DirectiveGet-chord-data tag))
		(if (not text)	(set! text "ral."))
            
        (if (d-Directive-chord? tag)
          (cond 
            ((not params)
                (set! params 'delete))
            ((list? params)
                (let ((type (car (list-ref params 0)))   (params (cdr (list-ref params 0))))
                    (if (eq? type 'direction)
                        (set! direction params)
                        (begin
                            (set! direction #f)
                            (d-WarningDialog  (_ "Sorry, not yet implemented"))))))
            ((string? params)
                        (set! text params))
            ((eq? params 'edit)
                (set! direction (GetLilyPondDirection)))
            ((eq? params 'advanced)
                    (if (not (d-DirectiveTextEdit-chord tag))
                        (set! params 'delete)))
            (else
                    (set! text (d-DirectiveGet-chord-display tag)))))
        (if (eq? params 'delete)
             (begin
                (d-DirectiveDelete-chord tag)
                (d-InfoDialog (_ "Text Span Start deleted. The end text span later should also be deleted ...")))
            (if direction
                (begin 
                    (set! direction (cond ((equal? direction "^") "\\textSpannerUp")((equal? direction "_") "\\textSpannerDown")(else "")))
                    (set! text (d-GetUserInputWithSnippets (_ "Text") (_ "Give text to appear with note/chord:\nThe characters \\, \", §, { and } have a special meaning in the text,\nthe backslash \\ starts some LilyPond syntax, the others must be paired.\nTo apply italic or bold to a group of words enclose them in {}, e.g. \\bold {These words are bold}.\nOther markup commands \\super, \\tiny etc, see LilyPond documentation.") text))
                    (if text
                            (begin
                               (set! text (car text))
                                (d-DirectivePut-chord-prefix tag  (string-append
                                            (string-append direction "\\override TextSpanner.bound-details.left.text = \\markup {" text "}  ")))
                                        (d-DirectivePut-chord-postfix tag  "\\startTextSpan")
                                        (d-DirectivePut-chord-override tag DENEMO_OVERRIDE_AFFIX)
                                        (d-DirectivePut-chord-data tag  text)
                                        (d-DirectivePut-chord-display tag  text))))))
        (d-SetSaved #f)))
        (set! TextSpanStart::params #f)
