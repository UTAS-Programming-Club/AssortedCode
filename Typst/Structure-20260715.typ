// Uses Typst 0.14.2

// Imports
#import "@preview/hydra:0.6.2": hydra


// Definitions
#let PC = "Programming Club"
#let required_exec_count = 2 // Only used for committee meeting minutes
#let required_voting_member_count = 15 // Only used for general meeting minutes
#let required_voting_member_percentage = 10 // Only used for general meeting minutes

// Use on first mention in general meeting agendas and minute attendances
// Removed legal names in public releases so switched to match full
#let amari_legal = "Amari Hanson"
#let cameron_legal =  "Cameron Munro"
#let joshua_legal = "Joshua Wierenga"
#let lachlan_legal = "Lachlan McKay"
#let liam_legal = "Liam Kearney"
#let luke_legal = "Luke, Hau Duc"

// Use on first mention in committee meeting agendas and minute attendances
#let amari_full = "Amari Hanson"
#let cameron_full =  "Cameron Munro"
#let joshua_full = "Joshua Wierenga"
#let lachlan_full = "Lachlan McKay"
#let liam_full = "Liam Kearney"
#let luke_full = "Luke, Hau Duc"

// Use on other mentions in committee meeting agenda and minutes
#let amari = "Amari"
#let cameron = "Cameron"
#let joshua = "Joshua"
#let lachlan = "Lachlan"
#let liam = "Liam"
#let luke = "Luke"

#let optional_attendance = "(Not required to attend)"


// Internal helpers
#let get_heading_format(it) = {
  let numbers = counter(heading).at(it.location())
  let number = numbering(it.numbering, ..numbers)
  it.supplement + " " + number + ": " + it.body
}

// I have no clue why datetime.display cannot do this
// TODO: Use nth plugin?
#let get_day_suffix(date) = {
  let day = str(date.day())
  if day.ends-with("1") and day != "11" {
    "st"
  } else if day.ends-with("2") and day != "12" {
    "nd"
  } else if day.ends-with("3") and day != "13" {
    "rd"
  } else {
    "th"
  }
}


// Formatting helpers
#let get_formatted_date(date) = {
  date.display("[day padding:none]") + super(get_day_suffix(date)) + " "
  date.display("[month repr:long] [year]")
}

// Prefer get_safe_formatted_time for times commonly changed
#let get_formatted_time(time) = {
  time.display("[hour padding:none repr:12]:[minute] [period]")
}

#let get_formatted_safe_time(time, wrap_time) = {
  if wrap_time and time.hour() < 9 {
    time += duration(hours: 12)
  }
  get_formatted_time(time)
}

// Only used for minutes
#let get_formatted_full_safe_datetime(datetime, wrap_time) = {
  datetime.display("[weekday] ")
  get_formatted_date(datetime) + " "
  get_formatted_safe_time(datetime, wrap_time)
}

// Only used for agenda
#let appendix_counter = counter("appendix_counter")
#let appendix(letter) = h(1fr) + underline("Appendix " + letter)

// Only used for agenda
#let received_email(sender, title, date) = [
  #sender — #date.display("[year][month][day]") — #title\
]

// Only used for agenda
#let sent_email(sender, recipient, title, date) = [
  #sender — #recipient — #date.display("[year][month][day]") — #title\
]

// Only used for agenda
#let simple_meeting_type(full_type) = {
  if full_type.ends-with("Committee") {
    return "Committee"
  } else if full_type.ends-with("General") {
    return "General"
  } else {
    return full_type
  }
}

// Only used for agenda
#let display_pdf(path, page_count) = {
  let pdf = read(path, encoding: none)
  place(scale(104%, image(pdf, scaling: "smooth"), reflow: true))
  grid(..range(page_count - 1).map(
    p => scale(106%, image(pdf, page: p + 2, scaling: "smooth")),
  ))
  if page_count == 1 {
    pagebreak()
  }
}

// Only used for minutes
#let placeholder_text(str) = text(fill: rgb(192, 0, 0), str)

// Only used for minutes
#let motion(mover: placeholder_text[Mover's name], seconder: placeholder_text[Seconder's name], action) = [
  *#underline[MOTION] #strong[Moved:]* #mover *Seconded:* #seconder #h(1em) #action
]

// Only used for minutes
// TODO: Avoid needing two placeholders?
#let group_motion(movers: (placeholder_text[Mover's name], placeholder_text[Mover's name]), action) = [
  #let sep = ","
  #if type(movers) == type(("")) and movers.join("").contains(",") {
    sep = ";"
  }

  *#underline[MOTION] Movers:* #movers.join(sep + " ", last: sep + " and ") #h(1em) #action
]

// Only used for general meeting minutes
#let office_motion(position, nominee: placeholder_text[Nominee's name], mover: placeholder_text[Mover's name], seconder: placeholder_text[Seconder's name]) = columns(3, gutter: 0pt)[
  #position - #nominee #colbreak()
  *Moved:* #mover #colbreak()
  *Seconded:* #seconder
]

// Only used for meeting minutes
#let quorem_check(general_meeting, voting_member_count, current_voting_member_count, waive_quorum, acknowledge_failing_quorum, second_last_cm_officer_count, last_cm_officer_count, second_last_cm_date, last_meeting_date, current_officer_count, current_exec_count) = {
  let meet_quorum = true
  
  if not general_meeting [
    #let required_officer_count = calc.ceil((second_last_cm_officer_count + last_cm_officer_count)/(2*required_exec_count))
      
    By Subrules 15.1 and 15.2 of the #PC Constitution, meeting business can only be conducted once at least half of the mean average of the number of officers at the previous two meetings, rounded up and at least #required_exec_count executives are present.
    
    At the second last committee meeting on #get_formatted_date(second_last_cm_date), we had #second_last_cm_officer_count officer(s) attending.\
    At the last committee meeting on #get_formatted_date(last_meeting_date), we had #last_cm_officer_count officer(s) attending.
    
    As $#current_officer_count$ officers and $#current_exec_count$ executives are present, $ceil((#second_last_cm_officer_count + #last_cm_officer_count)/(2×#required_exec_count)) = #required_officer_count$
    #{      
      if current_officer_count < required_officer_count {
        [exceeds the number of officers and so ]
        meet_quorum = false
      } else if current_officer_count == required_officer_count [
        matches the number of officers 
      ] else [
        is less than the number of officers 
      ]
      
      if meet_quorum and current_exec_count < required_exec_count {
        [but there are not enough executives so ]
        meet_quorum = false
      } else if meet_quorum [
        and there are enough executives, 
      ]
        
      if meet_quorum {
        [we can cover meeting business.]
      } else {
        [we cannot cover meeting business.]
      }
    }
  ] else [
    #let actual_required_voting_member_count = calc.min(required_voting_member_count, calc.ceil(voting_member_count / required_voting_member_percentage))
  
    By Subrule 19.2 of the #PC Constitution, meeting business can only be conducted once 
    $#required_voting_member_count$ or $#required_voting_member_percentage%$ of voting members, rounded up are present.
    
    In this case, $#actual_required_voting_member_count$ voting members are required and $#current_voting_member_count$ are present, so 
    #{
      let waive_quorum_text = "cannot"
      let waive_quorum_ending = "."
      if waive_quorum {
        waive_quorum_text = "would not be able to"
        waive_quorum_ending = ";"
      }
      
      if current_voting_member_count >= actual_required_voting_member_count {
        [we can cover meeting business.]
      } else {
        [we #waive_quorum_text cover meeting business]
        meet_quorum = false
      }
    }
    #if waive_quorum {
      [but, by Subrule 19.4, business may be conducted anyway.]
      meet_quorum = true
    }
  ]

  assert(meet_quorum or acknowledge_failing_quorum, message: "Quorum not met")
}


// Template
#let template(meeting_date, wrap_meeting_date, meeting_type, location, doc, doc_type) = {
  // Basic structure
  set document(title: PC + " Agenda " + meeting_date.display("[year][month][day]"))
  set text(font: "Atkinson Hyperlegible")
  set heading(numbering: "1.1")
  set text(lang: "eng", region: "AU")

  let meeting_type_heading = ""
  let held_heading = ""
  if doc_type == "agenda" {
    meeting_type_heading = "Agenda"
    held_heading = "to be held"
  } else if doc_type == "minutes" {
    meeting_type_heading = "Minutes"
    held_heading = "held"
  } else {
    assert(false)
  }
  
  
  // Page and heading structure
  set page("a4", margin: (x: 4em),
    header: context({
      if here().page() != 1 {
        emph(hydra(1, display: (_, it) => get_heading_format(it)))
        line(length: 100%)
      }
    }),
    footer: context({
      grid(
        columns: (1fr, 1fr),
        align: (left, right),
        stroke: none,
        place(horizon + left, image("/2025-2026/Logos/2021-2026 TUSA Logo Small.png", height: 1.12cm)),
        "Page " + counter(page).display(both: true, "1 of 1")
      )
    })
  )

  show heading.where(level: 1): it => par(get_heading_format(it))
  show heading.where(level: 2): set heading(supplement: [Item]) if doc_type == "agenda"

  
  // Page 1 heading
  grid(
    columns: (1fr, 1fr),
    align: (left, right),
    place(horizon + left, image("/2025-2026/Logos/2021-2026 TUSA Logo Large.png", height: 2.5cm)),
    strong[
      #PC\
      #meeting_type Meeting\
      #meeting_type_heading
    ]
  )
  linebreak()
  align(center, [
    For the #meeting_type Meeting
    #held_heading on #get_formatted_date(meeting_date)
    #location
    at #get_formatted_safe_time(meeting_date, wrap_meeting_date)
  ])
  line(length: 100%)

  
  // Document
  doc
}

#let agenda(meeting_date, wrap_meeting_date, meeting_type, location, agenda) = template(meeting_date, wrap_meeting_date, meeting_type, location, agenda, "agenda")

#let minutes(meeting_date, wrap_meeting_date, meeting_type, location, minutes) = template(meeting_date, wrap_meeting_date, meeting_type, location, minutes, "minutes")
