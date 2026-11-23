# Basic Auto-balancing

Status: New

## Story

Players find that most content is of roughly equal value. If content is too popular, compared to other content,
it is graudually and automatically reduced in effectiveness. Similarly, content that is unpopular gets gradually
and automatically improved.

## Design

The auto-balancing system is based on three concepts...

* Metrics
* Targets
* Dials

Metrics are things that can be measured or calculated from what exists or happens in the game. For example, the
number of each type of monster that is killed. Metrics can have multiple dimensions, such as the type of monster
or the biome on which it is killed. Raw metrics are measured directly from the game; derived metrics are 
calculated from other metrics.

A target for a metric is the value that the game designer would like the metric to have. 

Dials are things that the auto-balancing system can change to influence the value of a metric so that it gets
closer to the target.

## Worked Example

* metric_1 (raw) is the number of Raptors killed
* metric_2 (raw) is the number of Ants killed
* metric_3 (derived) is the total number of monsters killed
* metric_4 (derived) is the metric_1 / metric_3, the proportion of monster kills that are Raptors
* target_1 aims for metric_4 to be 60%
* dial_1 increases the armour of Raptors if metric_4 is above target_1, or decreases it if it is below

## Acceptance

Not written yet.
