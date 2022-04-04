<!--
  WeatherBoard.vue
  Created By: DowscopeMedia
  URL: http://dowscopemedia.ca/development
  Date: April 4, 2022
  Version: 1.0
  Desc: This is a simple weather component.  It uses Open Weather API, but
        it should work with other API's.  The styling utilizes BULMA and is needed to
        function properly.
-->
<template>
  <div class="weatherboard">
    <div class="card" v-if="weather!=null">
      <div class="media">
      <div class="media-left">
        <figure class="image is-48x48 has-background-info">
          <img :src="currentIcon" alt="">
        </figure>
      </div>
      <div class="media-content">
        <p class="title is-4">{{weather.weather[0].main}}</p>
        <p class="subtitle">{{weather.name}}</p>
      </div>
      <div class="media-right">
        <h1>{{currentTemp}}&#8451;</h1>
      </div>
    </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'WeatherBoard',
  data() {
    return {
      ipAddress: 0,
      weather: null,
    };
  },
  computed: {
    currentTemp() {
      return Math.floor(this.weather.main.temp ? !null : 0);
    },
    currentIcon() {
      let url = 'http://openweathermap.org/img/wn/';
      if (this.weather != null) {
        url = url.concat(this.weather.weather[0].icon).concat('@2x.png');
      } else {
        url = url.concat('1d0').concat('@2x.png');
      }
      return url;
    },
  },
  methods: {
    getWeather() {
      const API_URL = 'http://api.openweathermap.org';
      const API_KEY = ''; // Enter your openweather API Key
      const API_CITY = ''; // Enter the city
      const API_UNITS = ''; // Enter the units of measure. ie. metric
      const view = this;
      fetch(API_URL
        .concat('/geo/1.0/direct?q=')
        .concat(API_CITY)
        .concat('&limit=5&appid=')
        .concat(API_KEY))
        .then((res) => {
          res.json().then((data) => {
            fetch(API_URL
              .concat('/data/2.5/weather?lat=')
              .concat(data[0].lat)
              .concat('&lon=')
              .concat(data[0].lon)
              .concat('&appid=')
              .concat(API_KEY)
              .concat('&units=')
              .concat(API_UNITS))
              .then((response) => {
                response.json().then((data2) => {
                  view.weather = data2;
                });
              });
          });
        });
    },
  },
  mounted() {
    // Get the weather as soon as component is mounted.
    this.getWeather();
  },
};
</script>

<style scoped>
.weatherboard {
  width: 15rem;
  margin: 0.5rem;
}
.media-content {
  display: block;
  line-height: 10rem;
}
.media-right {
  font-size: 2rem;
  margin-right: 1rem;
}
.subtitle {
  font-size: 0.75rem;
}
</style>
