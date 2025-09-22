let map, marker, circle, autocomplete;

function initMap() {
    map = new google.maps.Map(document.getElementById("map"), {
        center: { lat: 40.6782, lng: -73.9442 },
        zoom: 15,
    });

    autocomplete = new google.maps.places.Autocomplete(document.getElementById("search"));
    autocomplete.addListener("place_changed", () => {
        const place = autocomplete.getPlace();
        if (place.geometry) {
            map.setCenter(place.geometry.location);
            marker.setPosition(place.geometry.location);
        }
    });

    marker = new google.maps.Marker({
        map: map,
        position: { lat: 40.6782, lng: -73.9442 },
        draggable: true,
    });

    circle = new google.maps.Circle({
        map: map,
        radius: 200,
        fillColor: "#FF0000",
        fillOpacity: 0.2,
        strokeColor: "#FF0000",
        strokeOpacity: 0.5,
    });
    circle.bindTo("center", marker, "position");

    document.getElementById("radius").addEventListener("change", (e) => {
        circle.setRadius(parseInt(e.target.value));
    });

    document.getElementById("fetch-btn").addEventListener("click", () => {
        const lat = marker.getPosition().lat();
        const lng = marker.getPosition().lng();
        const radius = parseInt(document.getElementById("radius").value);
        fetch("/fetch_data", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ lat, lng, radius }),
        });
    });
}

window.initMap = initMap;
