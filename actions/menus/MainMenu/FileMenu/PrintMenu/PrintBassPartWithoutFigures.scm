;;;;;;;;PrintBassPartWithoutFigures
(let ((tag "PrintBassPartWithoutFigures"))
   (define (HideOtherStaffs)
   	 (ForAllMovements  "(ForAllStaffs \"
                                                (if (not (d-HasFigures))
                                                    (d-NonPrintingStaff 'set)) 
                                                \")"))
       (define (ShowOtherStaffs)
   	 (ForAllMovements  "(ForAllStaffs \"
                                                (if (not (d-HasFigures))
                                                    (d-NonPrintingStaff 'unset)) 
                                                \")"))                                  
    (let ((name (_ "Bass without figures")))
	(d-DeleteLayout name)  
	(d-SelectDefaultLayout) 
	(d-ToggleFigures 'noninteractive)
        (HideOtherStaffs)
	(d-RefreshLilyPond)
	(d-CreateLayout name)   
	(d-SelectDefaultLayout)
	(d-ToggleFigures 'noninteractive)
        (ShowOtherStaffs)
	(d-WarningDialog (string-append (_ "Use Typeset->") name (_ " in the Print View to typeset your new layout")))
	(d-SetSaved #f)))
