;;;SetPageSize
(let ((tag "SetPageSize")(width #f)(height #f)(params SetPageSize::params))
    (define data (d-DirectiveGet-paper-data tag))
    (define (set-size thedata)
        (d-DirectivePut-paper-postfix tag (string-append "
#(set! paper-alist (cons '(\"custom-size\" . (cons (* " (car thedata) " cm) (* " (cdr thedata) " cm))) paper-alist))
#(set-paper-size \"custom-size\")"))
        (d-DirectivePut-paper-data tag (format #f "(cons ~s ~s)" width height)))
    (define (do-resize data)
        (set! width (d-GetUserInput (_ "Page Size") (_ "Give page width in cm ") (car data)))
        (if width
            (begin
                (set! height (d-GetUserInput (_ "Page Size") (_ "Give page height in cm ") (cdr data)))
                (if height
                    (set-size (cons width height))))))
     (if (not (d-Directive-paper? tag))
		(d-DirectivePut-paper-display tag (_ "Default Paper Size (A4)")))	      
    (if data
        (set! data (eval-string data)))
    (if (equal? params "edit")
        (set! params #f))
    (if (string? params)
        (set-size (eval-string params))
        (begin
            (if data
                    (begin
                        (let ((choice (RadioBoxMenu (cons (_ "Choose Size") 'resize)(cons "A4" 'default))))
                            (case choice
                                ((default)  (d-DirectivePut-paper-display tag (_ "Default Paper Size (A4)"))
											(set-size (cons "21.0" "29.7")));A4
                                ((resize) 
                                    (do-resize data)))))
                    (do-resize (cons "21.0" "29.7")))))) 
(d-SetSaved #f)
                                    
